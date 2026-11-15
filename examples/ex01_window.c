// Lesson 1: Minimal Raylib window, delta-time movement, and 60 FPS loop
#include "raylib.h"
#include <math.h>

int main(void) {
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    InitWindow(960, 540, "Ex 01: Raylib Window & Delta Time");
    SetTargetFPS(60);

    Vector2 pos = { 480.0f, 270.0f };
    float speed = 260.0f;

    while (!WindowShouldClose()) {
        float dt = fminf(GetFrameTime(), 0.05f);

        float mx = 0.0f, my = 0.0f;
        if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP))    my -= 1.0f;
        if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN))  my += 1.0f;
        if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT))  mx -= 1.0f;
        if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) mx += 1.0f;

        float len = hypotf(mx, my);
        if (len > 0.0f) {
            pos.x += (mx / len) * speed * dt;
            pos.y += (my / len) * speed * dt;
        }

        BeginDrawing();
        ClearBackground((Color){ 10, 12, 18, 255 });
        DrawCircleV(pos, 18.0f, (Color){ 214, 69, 60, 255 });
        DrawText("WASD / Arrows to move (normalized diagonal)", 20, 20, 20, RAYWHITE);
        DrawText(TextFormat("FPS: %d  dt: %.4f", GetFPS(), dt), 20, 48, 18, LIGHTGRAY);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
