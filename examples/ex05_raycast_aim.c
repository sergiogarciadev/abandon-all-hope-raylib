// Lesson 5: Mouse raycast aiming, tracer bullets, and ejecting brass casings
#include "raylib.h"
#include "raymath.h"
#include <math.h>
#include <stdlib.h>

#define MAX_B 64
#define MAX_C 64

typedef struct { bool on; Vector3 pos, dir; float life; } Bullet;
typedef struct { bool on; Vector3 pos, vel; float rot, life; } Casing;

int main(void) {
    InitWindow(960, 540, "Ex 05: Mouse Raycast Aiming, Tracers & Brass Casings");
    SetTargetFPS(60);

    Camera3D cam = { .position = { 0, 18, 9 }, .target = { 0, 0, 0 }, .up = { 0, 1, 0 }, .fovy = 50, .projection = CAMERA_PERSPECTIVE };
    Bullet bullets[MAX_B] = {0};
    Casing casings[MAX_C] = {0};
    Vector3 aimPt = { 0, 0.9f, 4 };
    float yaw = 0.0f, fireCd = 0.0f;

    while (!WindowShouldClose()) {
        float dt = fminf(GetFrameTime(), 0.05f);
        Ray ray = GetScreenToWorldRay(GetMousePosition(), cam);
        if (fabsf(ray.direction.y) > 1e-5f) {
            float t = (0.9f - ray.position.y) / ray.direction.y;
            if (t > 0.0f) {
                aimPt = Vector3Add(ray.position, Vector3Scale(ray.direction, t));
                yaw = atan2f(aimPt.x, aimPt.z);
            }
        }

        fireCd -= dt;
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && fireCd <= 0.0f) {
            fireCd = 0.13f;
            Vector3 fwd = { sinf(yaw), 0.0f, cosf(yaw) };
            Vector3 right = { cosf(yaw), 0.0f, -sinf(yaw) };
            for (int i = 0; i < MAX_B; i++) if (!bullets[i].on) {
                bullets[i] = (Bullet){ true, Vector3Add((Vector3){ 0, 0.9f, 0 }, Vector3Scale(fwd, 1.1f)), fwd, 0.9f };
                break;
            }
            for (int i = 0; i < MAX_C; i++) if (!casings[i].on) {
                casings[i] = (Casing){ true, (Vector3){ 0, 0.95f, 0 },
                    (Vector3){ right.x * 3.0f, 3.2f, right.z * 3.0f }, 0.0f, 2.2f };
                break;
            }
        }

        for (int i = 0; i < MAX_B; i++) if (bullets[i].on) {
            bullets[i].pos = Vector3Add(bullets[i].pos, Vector3Scale(bullets[i].dir, 36.0f * dt));
            bullets[i].life -= dt;
            if (bullets[i].life <= 0.0f) bullets[i].on = false;
        }
        for (int i = 0; i < MAX_C; i++) if (casings[i].on) {
            casings[i].vel.y -= 18.0f * dt;
            casings[i].pos = Vector3Add(casings[i].pos, Vector3Scale(casings[i].vel, dt));
            if (casings[i].pos.y < 0.03f) { casings[i].pos.y = 0.03f; casings[i].vel.y *= -0.35f; casings[i].vel.x *= 0.6f; casings[i].vel.z *= 0.6f; }
            casings[i].rot += 12.0f * dt;
            casings[i].life -= dt;
            if (casings[i].life <= 0.0f) casings[i].on = false;
        }

        BeginDrawing();
        ClearBackground((Color){ 10, 12, 16, 255 });
        BeginMode3D(cam);
        DrawGrid(20, 1.0f);
        DrawCube((Vector3){ 0, 0.7f, 0 }, 0.7f, 1.4f, 0.5f, (Color){ 110, 130, 150, 255 });
        DrawLine3D((Vector3){ 0, 0.9f, 0 }, aimPt, (Color){ 214, 69, 60, 140 });
        for (int i = 0; i < MAX_B; i++) if (bullets[i].on) {
            DrawCylinderEx(Vector3Subtract(bullets[i].pos, Vector3Scale(bullets[i].dir, 0.9f)), bullets[i].pos, 0.04f, 0.04f, 4, GOLD);
        }
        for (int i = 0; i < MAX_C; i++) if (casings[i].on) {
            DrawCube(casings[i].pos, 0.06f, 0.04f, 0.14f, (Color){ 232, 195, 90, 255 });
        }
        EndMode3D();
        DrawText("Move mouse to aim, hold Left Click to fire tracers & eject casings", 20, 20, 18, RAYWHITE);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
