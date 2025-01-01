#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <math.h>
#include <time.h>
#include <string.h>

#include <iostream>
#include <vector>

#include <fstream>
#include <sstream>

#include <raylib.h>
#include <raymath.h>

#define RAYGUI_IMPLEMENTATION
#include "raygui.hpp"

int main()
{

    InitWindow(400, 400, "Raylib osu!");
    SetTargetFPS(120);

    InitAudioDevice();

    Sound key_press_1 = LoadSound("key-press-1.mp3");

    while (!WindowShouldClose())
    {
        BeginDrawing();

        ClearBackground(GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR)));

        DrawText("Raylib osu!", 130, 100, 30, BLACK);

        if (GuiButton((Rectangle){100, 175, 50, 50}, "start"))
        {
            system("del input.wav");
            system("del output.wav");
            system("ls");
            system("ffmpeg -i input.mp3 input.wav");
            system("ffmpeg -i input.wav -ar 44100 output.wav");
            system("wav_to_beats.exe output.wav > output_beats.txt");
            break;
        }

        if (GuiButton((Rectangle){250, 175, 50, 50}, "close"))
        {
            CloseWindow();
        }

        EndDrawing();
    }

    InitWindow(1280, 720, "Raylib osu!");
    SetTargetFPS(120);

    DisableCursor();

    // Load the music file
    Music music = LoadMusicStream("output.wav");

    // Start playing the music
    PlayMusicStream(music);

    int score = 0;
    int streak = 0;
    float time_diff = 0.0f;

    float pre_x = 1280.0f / 2;
    float pre_y = 720.0f / 2;

    // Define a Circle struct
    struct Circle
    {
        Vector2 position;
        Color color;
        float radius;
        float elapsed_time;
        float max_time;
        float start_time;
    };

    // Create a list of circles
    std::vector<Circle> circles;

    // Clear the circles vector
    circles.clear();

    // Read circle data from a text file
    std::ifstream circleFile("output_beats.txt");
    std::string line;
    while (std::getline(circleFile, line))
    {
        std::istringstream iss(line);
        float x, y, time;
        if (!(iss >> x >> y >> time))
        {
            break;
        }

        float random_x = (float)GetRandomValue(-100, 100);
        float random_y = (float)GetRandomValue(-100, 100);

        pre_x += random_x;
        pre_y += random_y;

        // Playable area is 1280x720
        if (pre_x < 0)
        {
            pre_x = 0;
        }
        if (pre_x > 1280)
        {
            pre_x = 1280;
        }
        if (pre_y < 0)
        {
            pre_y = 0;
        }
        if (pre_y > 720)
        {
            pre_y = 720;
        }

        Circle circle;
        circle.position = {pre_x, pre_y};
        circle.radius = 30.0f;
        circle.color = (Color){
            (unsigned char)GetRandomValue(50, 255),
            (unsigned char)GetRandomValue(50, 255),
            (unsigned char)GetRandomValue(50, 255),
            // Start with alpha = 0
            0};
        circle.elapsed_time = 0.0f;
        circle.max_time = time;
        // Appear 1 seconds before end
        circle.start_time = circle.max_time - 1.0f;
        circles.push_back(circle);
    }

    // Add this variable to track elapsed time
    float elapsed_time = 0.0f;

    while (!WindowShouldClose())
    {
        float dt = GetFrameTime();
        elapsed_time += dt;

        // Update circles
        for (auto it = circles.begin(); it != circles.end();)
        {
            it->elapsed_time += dt;
            if (it->elapsed_time >= it->max_time)
            {
                it = circles.erase(it);
            }
            else
            {
                if (it->elapsed_time >= it->start_time)
                {
                    // Update alpha based on elapsed time since start_time
                    float alpha = ((it->elapsed_time - it->start_time) / (it->max_time - it->start_time)) * 255;
                    it->color.a = (unsigned char)alpha;
                }
                ++it;
            }
        }

        // Check for click and remove circles
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) || IsMouseButtonPressed(MOUSE_RIGHT_BUTTON) || IsKeyPressed(KEY_Z) || IsKeyPressed(KEY_X) || IsKeyPressed(KEY_SPACE))
        {
            Vector2 mouse = GetMousePosition();
            for (auto it = circles.begin(); it != circles.end();)
            {
                if (it->elapsed_time >= it->start_time && CheckCollisionPointCircle(mouse, it->position, it->radius))
                {
                    // Calculate time difference between click and circle's max_time
                    time_diff = fabs(it->max_time - elapsed_time);

                    std::cout << "Time difference: " << time_diff << std::endl;

                    int points = 300 - (int)(fabs(time_diff) * 100);

                    std::cout << "Points: " << points << std::endl;

                    score += points;
                    streak += 1;

                    it = circles.erase(it);

                    PlaySound(key_press_1);
                }
                else
                {
                    ++it;
                }
            }
        }

        // Update the music stream
        UpdateMusicStream(music);

        // Drawing
        BeginDrawing();
        {
            ClearBackground((Color){30, 30, 30, 255});

            // Draw all visible circles
            for (const auto &circle : circles)
            {
                if (circle.elapsed_time >= circle.start_time)
                {
                    // Draw the circle
                    DrawCircleV(circle.position, circle.radius, circle.color);

                    // Calculate and draw the shrinking ring
                    float t = (circle.elapsed_time - circle.start_time) / (circle.max_time - circle.start_time);
                    float ring_radius = circle.radius + circle.radius * 0.5f * (1.0f - t); // Starts larger
                    DrawCircleLines((int)circle.position.x, (int)circle.position.y, ring_radius, WHITE);
                }
            }

            if (GuiButton((Rectangle){1280 - 100, 720 - 100, 50, 50}, "close"))
            {
                CloseWindow();
            }

            // Draw score
            DrawText(TextFormat("Score: %d", score), 250, 10, 20, WHITE);

            // Draw streak
            DrawText(TextFormat("Streak: %d", streak), 450, 10, 20, WHITE);

            // Draw Time Difference
            DrawText(TextFormat("Time difference: %.2f", time_diff), 600, 10, 20, WHITE);

            // Draw timer
            DrawText(TextFormat("Time: %.2f s", elapsed_time), 900, 10, 20, WHITE);

            // Draw mouse cursor
            Vector2 mouse = GetMousePosition();
            DrawCircle((int)mouse.x, (int)mouse.y, 10, WHITE);
        }
        EndDrawing();
    }

    // Unload the music
    UnloadMusicStream(music);
    // Close the audio device
    CloseAudioDevice();
    // Close the window
    CloseWindow();

    return 0;
}
