// Lesson 7: Storm rain streaks, ground splash rings, and double-exponential lightning flash
#include "raylib.h"
#include <math.h>
#include <stdlib.h>

#define RAIN_N 900
#define SPLASH_N 80

typedef struct { float x, y, z; } Drop;
typedef struct { float x, z, age; } Splash;

static float rnd01(void) { return (float)GetRandomValue(0, 10000) / 10000.0f; }

int main(void) {
    InitWindow(960, 540, "Ex 07: Storm Rain, Splashes & Lightning Double-Flash");
    SetTargetFPS(60);

    Camera3D cam = { .position = { 0, 18, 12 }, .target = { 0, 0, 0 }, .up = { 0, 1, 0 }, .fovy = 55, .projection = CAMERA_PERSPECTIVE };
    Drop rain[RAIN_N];
    Splash splashes[SPLASH_N];
    for (int i = 0; i < RAIN_N; i++) rain[i] = (Drop){ (rnd01() - 0.5f) * 32.0f, rnd01() * 14.0f, (rnd01() - 0.5f) * 26.0f };
    for (int i = 0; i < SPLASH_N; i++) splashes[i] = (Splash){ (rnd01() - 0.5f) * 24.0f, (rnd01() - 0.5f) * 18.0f, rnd01() * 0.4f };

    float flashT = -1.0f;

    while (!WindowShouldClose()) {
        float dt = fminf(GetFrameTime(), 0.05f);
        if (IsKeyPressed(KEY_SPACE)) flashT = 0.0f;

        float flash = 0.0f;
        if (flashT >= 0.0f) {
            flashT += dt;
            flash = expf(-flashT * 14.0f) + (flashT > 0.13f ? 0.8f * expf(-(flashT - 0.13f) * 9.0f) : 0.0f);
            if (flashT > 1.2f) flashT = -1.0f;
        }

        for (int i = 0; i < RAIN_N; i++) {
            rain[i].y -= 22.0f * dt;
            if (rain[i].y < 0.0f) rain[i].y += 14.0f;
        }
        for (int i = 0; i < SPLASH_N; i++) {
            splashes[i].age += dt;
            if (splashes[i].age > 0.35f) {
                splashes[i] = (Splash){ (rnd01() - 0.5f) * 24.0f, (rnd01() - 0.5f) * 18.0f, 0.0f };
            }
        }

        Color sky = {
            (unsigned char)(5 + fminf(1.0f, flash * 0.7f) * 53.0f),
            (unsigned char)(6 + fminf(1.0f, flash * 0.7f) * 62.0f),
            (unsigned char)(10 + fminf(1.0f, flash * 0.7f) * 78.0f),
            255
        };

        BeginDrawing();
        ClearBackground(sky);
        BeginMode3D(cam);
        DrawGrid(24, 1.0f);
        for (int i = 0; i < RAIN_N; i++) {
            Vector3 p = { rain[i].x, rain[i].y, rain[i].z };
            DrawLine3D(p, (Vector3){ p.x - 0.12f, p.y - 0.85f, p.z }, (Color){ 165, 195, 230, 120 });
        }
        for (int i = 0; i < SPLASH_N; i++) {
            float r = 0.06f + splashes[i].age * 0.8f;
            unsigned char a = (unsigned char)((1.0f - splashes[i].age / 0.35f) * 140.0f);
            DrawCircle3D((Vector3){ splashes[i].x, 0.02f, splashes[i].z }, r, (Vector3){ 1, 0, 0 }, 90.0f, (Color){ 170, 200, 235, a });
        }
        EndMode3D();
        DrawText("Press [SPACE] to trigger double-exponential lightning flash", 20, 20, 18, RAYWHITE);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
