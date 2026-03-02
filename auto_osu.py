import argparse
import math
import time
import subprocess
import os
from dataclasses import dataclass

import cv2
import numpy as np
import pyautogui

try:
	import pydirectinput
	HAS_PYDIRECTINPUT = True
except ImportError:
	HAS_PYDIRECTINPUT = False

try:
	import pygetwindow
	HAS_PYGETWINDOW = True
except ImportError:
	HAS_PYGETWINDOW = False


@dataclass
class BotConfig:
	fps: float
	min_radius: float
	max_radius: float
	circularity: float
	click_cooldown: float
	move_duration: float
	click_delay: float
	saturation_min: int
	value_min: int
	debug: bool
	use_keyboard: bool
	use_pydirectinput: bool
	focus_window: str | None
	region: tuple[int, int, int, int] | None


class AutoOsuBot:
	def __init__(self, config: BotConfig):
		self.config = config
		self.clicked_history: list[tuple[float, tuple[int, int]]] = []
		self.use_left = True
		self.total_clicks = 0
		self.send_key_path = self._find_send_key_exe()

	def _grab_frame(self) -> np.ndarray:
		image = pyautogui.screenshot(region=self.config.region)
		frame = np.array(image)
		return cv2.cvtColor(frame, cv2.COLOR_RGB2BGR)

	def _find_send_key_exe(self) -> str | None:
		"""Find send_key.exe in current directory or Debug/Release folders."""
		possible_paths = [
			"send_key.exe",
			"Debug/send_key.exe",
			"Release/send_key.exe",
			"./send_key.exe",
		]
		for path in possible_paths:
			if os.path.exists(path):
				return path
		return None

	def _send_key_native(self, key: str) -> bool:
		"""Send a key using the native send_key.exe program."""
		if not self.send_key_path:
			return False
		try:
			subprocess.run([self.send_key_path, key],
						   check=True,
						   capture_output=True,
						   timeout=0.1,
						   creationflags=subprocess.CREATE_NO_WINDOW if os.name == 'nt' else 0)
			return True
		except Exception:
			return False

	def _detect_targets(self, frame: np.ndarray) -> list[tuple[int, int, float]]:
		hsv = cv2.cvtColor(frame, cv2.COLOR_BGR2HSV)

		lower = np.array([0, self.config.saturation_min, self.config.value_min], dtype=np.uint8)
		upper = np.array([179, 255, 255], dtype=np.uint8)
		mask = cv2.inRange(hsv, lower, upper)

		kernel = np.ones((3, 3), np.uint8)
		mask = cv2.morphologyEx(mask, cv2.MORPH_OPEN, kernel, iterations=1)
		mask = cv2.morphologyEx(mask, cv2.MORPH_CLOSE, kernel, iterations=1)

		contours, _ = cv2.findContours(mask, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)

		candidates: list[tuple[int, int, float]] = []
		min_area = math.pi * (self.config.min_radius ** 2)
		max_area = math.pi * (self.config.max_radius ** 2)

		for contour in contours:
			area = cv2.contourArea(contour)
			if area < min_area or area > max_area:
				continue

			perimeter = cv2.arcLength(contour, True)
			if perimeter <= 0:
				continue

			circularity = 4.0 * math.pi * area / (perimeter * perimeter)
			if circularity < self.config.circularity:
				continue

			(x, y), radius = cv2.minEnclosingCircle(contour)
			if radius < self.config.min_radius or radius > self.config.max_radius:
				continue

			candidates.append((int(x), int(y), radius))

		return candidates

	def _is_recently_clicked(self, point: tuple[int, int], now: float) -> bool:
		keep_after = now - self.config.click_cooldown * 2.5
		self.clicked_history = [item for item in self.clicked_history if item[0] >= keep_after]

		for t, previous_point in self.clicked_history:
			if now - t > self.config.click_cooldown:
				continue

			distance = math.dist(point, previous_point)
			if distance < self.config.max_radius:
				return True

		return False

	def _to_screen_space(self, x: int, y: int) -> tuple[int, int]:
		if self.config.region is None:
			return x, y
		region_x, region_y, _, _ = self.config.region
		return region_x + x, region_y + y

	def _focus_window(self) -> bool:
		if not self.config.focus_window:
			return True
		if not HAS_PYGETWINDOW:
			print("WARNING: pygetwindow not installed. Cannot auto-focus window.")
			print("Install with: pip install pygetwindow")
			return False

		try:
			windows = pygetwindow.getWindowsWithTitle(self.config.focus_window)
			if not windows:
				print(f"ERROR: Window with title '{self.config.focus_window}' not found.")
				print("Make sure the Raylib osu game is running.")
				return False

			window = windows[0]
			print(f"Found window: {window.title}")
			window.activate()
			print(f"Window focused: {window.title}")
			return True
		except Exception as e:
			print(f"ERROR focusing window: {e}")
			return False

	def _render_debug_overlay(self, frame: np.ndarray, mask: np.ndarray, targets: list[tuple[int, int, float]], loop_fps: float):
		debug_frame = frame.copy()

		for x, y, radius in targets:
			cv2.circle(debug_frame, (x, y), int(radius), (0, 255, 0), 2)
			cv2.circle(debug_frame, (x, y), 2, (0, 0, 255), -1)

		if self.config.use_keyboard:
			input_mode = "keyboard (z/x)"
		elif self.config.use_pydirectinput:
			input_mode = "pydirectinput"
		else:
			input_mode = "pyautogui"

		cv2.putText(debug_frame, f"Targets: {len(targets)}", (10, 26), cv2.FONT_HERSHEY_SIMPLEX, 0.7, (255, 255, 255), 2)
		cv2.putText(debug_frame, f"Clicks: {self.total_clicks}", (10, 52), cv2.FONT_HERSHEY_SIMPLEX, 0.7, (255, 255, 255), 2)
		cv2.putText(debug_frame, f"FPS: {loop_fps:.1f} | {input_mode}", (10, 78), cv2.FONT_HERSHEY_SIMPLEX, 0.65, (255, 255, 255), 2)
		cv2.putText(debug_frame, "Press 'q' to stop", (10, 104), cv2.FONT_HERSHEY_SIMPLEX, 0.6, (180, 220, 255), 2)

		mask_bgr = cv2.cvtColor(mask, cv2.COLOR_GRAY2BGR)
		combined = np.hstack((debug_frame, mask_bgr))

		scale = 0.5
		small = cv2.resize(combined, (int(combined.shape[1] * scale), int(combined.shape[0] * scale)))
		cv2.imshow("auto_osu debug (left: frame, right: mask) - Press q to stop", small)

	def run(self):
		frame_delay = 1.0 / self.config.fps
		print("Auto osu bot starting...")
		print("Move mouse to top-left corner to trigger PyAutoGUI fail-safe and stop.")
		print("Press 'q' in debug window to stop.")

		if self.config.use_keyboard:
			if self.send_key_path:
				print(f"Input mode: keyboard (z/x) via native SendInput [{self.send_key_path}]")
			else:
				print("WARNING: send_key.exe not found. Build it with: cmake . && make")
				print("Input mode: keyboard (z/x) via pyautogui (may not work)")
		elif self.config.use_pydirectinput:
			if not HAS_PYDIRECTINPUT:
				print("ERROR: pydirectinput not installed. Install with: pip install pydirectinput")
				return
			print("Input mode: pydirectinput (direct OS input)")
		else:
			print("Input mode: pyautogui mouse clicks")

		if self.config.focus_window:
			print(f"Attempting to focus window: '{self.config.focus_window}'...")
			if not self._focus_window():
				print("Failed to focus window. Make sure the game is running and try manually.")
				return
			print("Starting in 3 seconds...")
		else:
			print("IMPORTANT: Make sure the Raylib game window is focused (click it).")
			print("Starting in 3 seconds...")
		time.sleep(3)

		try:
			while True:
				loop_start = time.perf_counter()
				frame = self._grab_frame()

				hsv = cv2.cvtColor(frame, cv2.COLOR_BGR2HSV)
				lower = np.array([0, self.config.saturation_min, self.config.value_min], dtype=np.uint8)
				upper = np.array([179, 255, 255], dtype=np.uint8)
				mask = cv2.inRange(hsv, lower, upper)

				kernel = np.ones((3, 3), np.uint8)
				mask = cv2.morphologyEx(mask, cv2.MORPH_OPEN, kernel, iterations=1)
				mask = cv2.morphologyEx(mask, cv2.MORPH_CLOSE, kernel, iterations=1)

				contours, _ = cv2.findContours(mask, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)
				targets: list[tuple[int, int, float]] = []
				min_area = math.pi * (self.config.min_radius ** 2)
				max_area = math.pi * (self.config.max_radius ** 2)

				for contour in contours:
					area = cv2.contourArea(contour)
					if area < min_area or area > max_area:
						continue

					perimeter = cv2.arcLength(contour, True)
					if perimeter <= 0:
						continue

					circularity = 4.0 * math.pi * area / (perimeter * perimeter)
					if circularity < self.config.circularity:
						continue

					(x, y), radius = cv2.minEnclosingCircle(contour)
					if radius < self.config.min_radius or radius > self.config.max_radius:
						continue

					targets.append((int(x), int(y), radius))

				now = time.perf_counter()
				targets.sort(key=lambda item: item[2])

				for x, y, _radius in targets:
					screen_x, screen_y = self._to_screen_space(x, y)
					point = (screen_x, screen_y)
					if self._is_recently_clicked(point, now):
						continue

					pyautogui.moveTo(screen_x, screen_y, duration=self.config.move_duration)
					if self.config.click_delay > 0:
						time.sleep(self.config.click_delay)

					if self.config.use_keyboard:
						key = "z" if self.use_left else "x"
					if self.send_key_path:
						self._send_key_native(key)
					else:
						pyautogui.press(key)
					self.use_left = not self.use_left
					self.total_clicks += 1
					self.clicked_history.append((time.perf_counter(), point))

				elapsed = time.perf_counter() - loop_start
				loop_fps = 1.0 / elapsed if elapsed > 0 else 0.0

				if self.config.debug:
					self._render_debug_overlay(frame, mask, targets, loop_fps)
					if cv2.waitKey(1) & 0xFF == ord('q'):
						print("Stopping bot: 'q' pressed in debug window.")
						break

				sleep_time = frame_delay - elapsed
				if sleep_time > 0:
					time.sleep(sleep_time)
		except pyautogui.FailSafeException:
			print("PyAutoGUI fail-safe triggered. Exiting.")
		except KeyboardInterrupt:
			print("Interrupted by user. Exiting.")
		finally:
			if self.config.debug:
				cv2.destroyAllWindows()


def parse_args() -> BotConfig:
	parser = argparse.ArgumentParser(description="Auto player for the Raylib osu clone using screen capture.")
	parser.add_argument("--fps", type=float, default=60.0, help="Capture loop FPS.")
	parser.add_argument("--min-radius", type=float, default=18.0, help="Minimum target radius in pixels.")
	parser.add_argument("--max-radius", type=float, default=40.0, help="Maximum target radius in pixels.")
	parser.add_argument("--circularity", type=float, default=0.68, help="Minimum contour circularity.")
	parser.add_argument("--cooldown", type=float, default=0.12, help="Seconds before clicking near same spot again.")
	parser.add_argument("--move-duration", type=float, default=0.0, help="Mouse move duration in seconds.")
	parser.add_argument("--click-delay", type=float, default=0.01, help="Delay in seconds between mouse move and click.")
	parser.add_argument("--sat-min", type=int, default=70, help="Minimum HSV saturation for target mask.")
	parser.add_argument("--val-min", type=int, default=60, help="Minimum HSV value for target mask.")
	parser.add_argument("--region", type=int, nargs=4, metavar=("X", "Y", "W", "H"), help="Optional screen region to scan.")
	parser.add_argument("--no-debug", action="store_true", help="Disable live debug overlay window.")
	parser.add_argument("--keyboard", action="store_true", help="Use keyboard inputs (z/x) instead of mouse clicks.")
	parser.add_argument("--pydirectinput", action="store_true", help="Use pydirectinput for direct OS mouse clicks (best for unregistered clicks).")
	parser.add_argument("--focus-window", type=str, default="Raylib osu!", help="Window title to auto-focus before starting (default: 'Raylib osu!'). Use empty string to disable.")
	args = parser.parse_args()
	region = tuple(args.region) if args.region else None
	focus_window = args.focus_window if args.focus_window else None

	return BotConfig(
		fps=args.fps,
		min_radius=args.min_radius,
		max_radius=args.max_radius,
		circularity=args.circularity,
		click_cooldown=args.cooldown,
		move_duration=args.move_duration,
		click_delay=args.click_delay,
		saturation_min=args.sat_min,
		value_min=args.val_min,
		debug=not args.no_debug,
		use_keyboard=args.keyboard,
		use_pydirectinput=args.pydirectinput,
		focus_window=focus_window,
		region=region,
	)


def main():
	pyautogui.PAUSE = 0.0
	pyautogui.FAILSAFE = True

	config = parse_args()
	bot = AutoOsuBot(config)
	bot.run()


if __name__ == "__main__":
	main()
