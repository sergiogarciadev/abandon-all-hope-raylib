// Lesson 4a: Asymmetric stance/swing foot_cycle() gait curve & pelvis bob
#include "raylib.h"
#include <math.h>

typedef struct { float s, lift, pitch; } FootPose;

static FootPose foot_cycle(float p, float beta) {
    if (p < beta) {
        float u = p / beta;
        return (FootPose){ 0.5f - u, 0.0f, 0.0f };
    }
    float u = (p - beta) / (1.0f - beta);
    return (FootPose){ -0.5f + 0.5f * (1.0f - cosf(PI * u)), sinf(PI * u), 0.35f * sinf(2.0f * PI * u) };
}

int main(void) {
    InitWindow(960, 540, "Ex 04a: Procedural Gait Cycle (foot_cycle)");
    SetTargetFPS(60);

    float phase = 0.0f;
    const float beta = 0.62f;

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        phase = fmodf(phase + 1.4f * dt, 1.0f);

        BeginDrawing();
        ClearBackground((Color){ 14, 16, 22, 255 });
        DrawLine(100, 390, 860, 390, GRAY);

        float bob = -12.0f * cosf(2.0f * (phase * 2.0f * PI - PI * beta));
        Vector2 hip = { 480.0f, 230.0f + bob };
        DrawCircleV(hip, 16, (Color){ 232, 210, 138, 255 });

        for (int i = 0; i < 2; i++) {
            float p = fmodf(phase + i * 0.5f, 1.0f);
            FootPose fp = foot_cycle(p, beta);
            Vector2 foot = { 480.0f + fp.s * 180.0f, 385.0f - fp.lift * 55.0f };
            Color col = (i == 0) ? (Color){ 214, 69, 60, 255 } : (Color){ 111, 168, 220, 255 };
            DrawLineEx(hip, foot, 6.0f, col);
            DrawCircleV(foot, 10, col);
        }

        DrawText("Asymmetric gait: 62% stance (planted) / 38% swing (lifted)", 20, 20, 18, RAYWHITE);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
