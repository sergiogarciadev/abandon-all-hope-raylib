#include "raylib.h"

int main(void) {
    const int screenWidth = 1280;
    const int screenHeight = 720;

    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    InitWindow(screenWidth, screenHeight, "Abandon All Hope");
    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground((Color){5, 6, 10, 255});
        DrawText("ABANDON ALL HOPE - Raylib + CMake + LLVM", 40, 40, 24, RAYWHITE);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
