// Lesson 6: Zombie AI state machine (Chase -> Bash Window -> 3-Phase Climb)
#include "raylib.h"
#include <math.h>
#include <stdbool.h>

typedef enum { AI_CHASE = 0, AI_BASH, AI_CLIMB } ZState;

int main(void) {
    InitWindow(960, 540, "Ex 06: Zombie AI State Machine & Window Climb");
    SetTargetFPS(60);

    Camera3D cam = { .position = { 0, 16, 12 }, .target = { 0, 0, 0 }, .up = { 0, 1, 0 }, .fovy = 50, .projection = CAMERA_PERSPECTIVE };
    Vector3 zPos = { 0, 0, 7.0f };
    Vector3 playerPos = { 0, 0, -4.0f };
    int winHp = 3;
    ZState st = AI_CHASE;
    float timer = 0.0f;

    while (!WindowShouldClose()) {
        float dt = fminf(GetFrameTime(), 0.05f);
        if (IsKeyPressed(KEY_R)) { zPos = (Vector3){ 0, 0, 7.0f }; winHp = 3; st = AI_CHASE; timer = 0.0f; }

        if (st == AI_CHASE) {
            if (zPos.z > 1.0f) {
                zPos.z -= 2.4f * dt;
            } else if (winHp > 0) {
                st = AI_BASH;
                timer = 0.0f;
            } else {
                st = AI_CLIMB;
                timer = 0.0f;
            }
        } else if (st == AI_BASH) {
            timer += dt;
            if (timer >= 0.7f) {
                timer = 0.0f;
                winHp--;
                if (winHp <= 0) st = AI_CLIMB;
            }
        } else if (st == AI_CLIMB) {
            timer += dt;
            float u = fminf(1.0f, timer / 1.2f);
            zPos.z = 1.0f - 2.2f * u;
            zPos.y = sinf(u * PI) * 1.15f; // Arc over window sill
            if (u >= 1.0f) { zPos.y = 0.0f; st = AI_CHASE; }
        }

        const char *stName = (st == AI_CHASE) ? "CHASE" : (st == AI_BASH ? "BASH WINDOW" : "CLIMBING OVER SILL");

        BeginDrawing();
        ClearBackground((Color){ 10, 12, 16, 255 });
        BeginMode3D(cam);
        DrawGrid(18, 1.0f);
        DrawCube((Vector3){ -3.5f, 1.2f, 0 }, 5.0f, 2.4f, 0.4f, BROWN);
        DrawCube((Vector3){  3.5f, 1.2f, 0 }, 5.0f, 2.4f, 0.4f, BROWN);
        DrawCube((Vector3){  0.0f, 0.4f, 0 }, 2.0f, 0.8f, 0.4f, DARKBROWN); // Sill
        if (winHp > 0) DrawCube((Vector3){ 0, 1.3f, 0 }, 2.0f, 1.0f, 0.1f, SKYBLUE);
        DrawCube((Vector3){ playerPos.x, 0.8f, playerPos.z }, 0.7f, 1.6f, 0.5f, GREEN);
        DrawCube((Vector3){ zPos.x, zPos.y + 0.8f, zPos.z }, 0.7f, 1.6f, 0.5f, RED);
        EndMode3D();
        DrawText(TextFormat("Zombie AI State: %s | Window HP: %d | [R] Reset", stName, winHp), 20, 20, 18, RAYWHITE);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
