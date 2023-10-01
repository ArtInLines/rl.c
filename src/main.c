#include <stdint.h>  // For specifically sized ints
#include <stdbool.h> // For boolean definitions
#include <stdio.h>   // For printf - only used for debugging
#include <assert.h>  // For assert
#include <string.h>  // For memcpy
#include "raylib.h"
#include "hpdf.h"

int main(void)
{
    int32_t win_width  = 1200;
    int32_t win_height = 600;

    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(win_width, win_height, "RL");
    SetTargetFPS(60);

    float size = 50;
    float spacing = 2;
    Font font = LoadFontEx("./assets/Roboto-Regular.ttf", size, NULL, 95);

    while (!WindowShouldClose() || IsKeyPressed(KEY_ESCAPE)) {
        BeginDrawing();

        if (IsWindowResized()) {
            win_width  = GetScreenWidth();
            win_height = GetScreenHeight();
        }
        ClearBackground(BLACK);

        char* text = "Reading List";
        Vector2 v = MeasureTextEx(font, text, size, spacing);
        DrawTextEx(font, text, (Vector2){.x = (win_width - v.x)/2, .y = (win_height - size)/2}, size, spacing, WHITE);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}