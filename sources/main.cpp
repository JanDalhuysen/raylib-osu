#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <math.h>
#include <time.h>

#include <string.h>
#include <vector>

#include <fstream>
#include <sstream>

#include <raylib.h>
#include <raymath.h>

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#include <iostream>

int main()
{

    InitWindow(400, 400, "hello world");
    SetTargetFPS(120);

    while (!WindowShouldClose())
    {
        BeginDrawing();

        ClearBackground(GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR)));

        DrawText("Raylib osu!", 130, 100, 30, BLACK);

        if (GuiButton((Rectangle){100, 175, 50, 50}, "start"))
        {
            break;
        }

        if (GuiButton((Rectangle){250, 175, 50, 50}, "close"))
        {
            CloseWindow();
        }

        EndDrawing();
    }

    InitWindow(400, 400, "osu");
    SetTargetFPS(120);

    DisableCursor();

    // Initialize the audio device
    InitAudioDevice();

    // Load the music file
    Music music = LoadMusicStream("funk_output.wav");

    // Start playing the music
    PlayMusicStream(music);

    int score = 0;

    // Define a Circle struct
    struct Circle
    {
        Vector2 position;
        float radius;
        Color color;
        float elapsed_time;
        float max_time;
        float start_time;
    };

    // Create a list of circles
    std::vector<Circle> circles;

    // Clear the circles vector
    circles.clear();

    // Read circle data from a text file
    std::ifstream circleFile("random_circles_uptown_funk.txt");
    std::string line;
    while (std::getline(circleFile, line))
    {
        std::istringstream iss(line);
        float x, y, time;
        if (!(iss >> x >> y >> time))
        {
            break;
        }

        float random_circle = (float)GetRandomValue(20, 50);
        x += random_circle;
        y += random_circle;

        Circle circle;
        circle.position = {x, y};
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

    // Generate random circles
    // for (int i = 0; i < 10; i++)
    // {
    //     Circle circle;
    //     circle.position = {(float)GetRandomValue(0, window_width), (float)GetRandomValue(0, window_height)};
    //     circle.radius = 30; //(float)GetRandomValue(20, 50);
    //     circle.color = (Color){
    //         (unsigned char)GetRandomValue(50, 255),
    //         (unsigned char)GetRandomValue(50, 255),
    //         (unsigned char)GetRandomValue(50, 255),
    //         0 // Start with alpha = 0
    //     };
    //     circle.elapsed_time = 0.0f;
    //     circle.max_time = (float)GetRandomValue(3, 6); // Time until fully opaque
    //     circles.push_back(circle);
    // }

    // Add this variable to track elapsed time
    float elapsed_time = 0.0f;

    while (!WindowShouldClose())
    {
        float dt = GetFrameTime();
        elapsed_time += dt; // Update elapsed time

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
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
        {
            Vector2 mouse = GetMousePosition();
            for (auto it = circles.begin(); it != circles.end();)
            {
                if (it->elapsed_time >= it->start_time && CheckCollisionPointCircle(mouse, it->position, it->radius))
                {
                    // Calculate time difference between click and circle's max_time
                    float time_diff = fabs(it->max_time - elapsed_time);

                    std::cout << "Time difference: " << time_diff << std::endl;

                    int points = 300 - (int)(fabs(time_diff) * 100);

                    std::cout << "Points: " << points << std::endl;

                    score += points;

                    it = circles.erase(it);
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

            if (GuiButton((Rectangle){300, 300, 50, 50}, "close"))
            {
                CloseWindow();
            }

            // Draw score
            DrawText(TextFormat("Score: %d", score), 10, 10, 20, WHITE);

            // Draw timer
            DrawText(TextFormat("Time: %.2f s", elapsed_time), 400 - 150, 10, 20, WHITE);

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

    CloseWindow();

    return 0;
}
