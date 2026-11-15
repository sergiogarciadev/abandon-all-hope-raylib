// Lesson 4b: 3D Two-Bone Inverse Kinematics (ik2) with pole vector
#include "raylib.h"
#include "raymath.h"
#include <math.h>

static Vector3 ik2(Vector3 a, Vector3 b, float l1, float l2, Vector3 pole) {
    Vector3 ab = Vector3Subtract(b, a);
    float d = Vector3Length(ab);
    if (d < 1e-4f) d = 1e-4f;
    float maxD = (l1 + l2) * 0.995f;
    float minD = fabsf(l1 - l2) + 1e-3f;
    if (d > maxD) d = maxD;
    if (d < minD) d = minD;

    Vector3 dir = Vector3Scale(ab, 1.0f / Vector3Length(ab));
    float cosA = (l1 * l1 + d * d - l2 * l2) / (2.0f * l1 * d);
    cosA = Clamp(cosA, -1.0f, 1.0f);
    float sinA = sqrtf(fmaxf(0.0f, 1.0f - cosA * cosA));

    Vector3 bend = Vector3Subtract(pole, Vector3Scale(dir, Vector3DotProduct(pole, dir)));
    if (Vector3LengthSqr(bend) < 1e-6f) bend = (Vector3){ 0, 1, 0 };
    bend = Vector3Normalize(bend);

    return Vector3Add(a, Vector3Add(Vector3Scale(dir, l1 * cosA), Vector3Scale(bend, l1 * sinA)));
}

int main(void) {
    InitWindow(960, 540, "Ex 04b: 3D Two-Bone IK Solver");
    SetTargetFPS(60);

    Camera3D cam = { .position = { 0, 2.2f, 4.5f }, .target = { 0, 0.8f, 0 }, .up = { 0, 1, 0 }, .fovy = 45, .projection = CAMERA_PERSPECTIVE };
    Vector3 shoulder = { -0.6f, 1.4f, 0.0f };
    Vector3 target = { 0.8f, 0.9f, 0.2f };
    Vector3 pole = { 0.0f, -1.0f, -0.8f };

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        if (IsKeyDown(KEY_W)) target.y += 1.6f * dt;
        if (IsKeyDown(KEY_S)) target.y -= 1.6f * dt;
        if (IsKeyDown(KEY_A)) target.x -= 1.6f * dt;
        if (IsKeyDown(KEY_D)) target.x += 1.6f * dt;
        if (IsKeyDown(KEY_Q)) target.z -= 1.6f * dt;
        if (IsKeyDown(KEY_E)) target.z += 1.6f * dt;

        Vector3 elbow = ik2(shoulder, target, 1.1f, 1.0f, pole);

        BeginDrawing();
        ClearBackground((Color){ 14, 16, 22, 255 });
        BeginMode3D(cam);
        DrawGrid(10, 0.5f);
        DrawCylinderEx(shoulder, elbow, 0.08f, 0.06f, 8, (Color){ 232, 210, 138, 255 });
        DrawCylinderEx(elbow, target, 0.06f, 0.04f, 8, (Color){ 143, 174, 107, 255 });
        DrawSphere(shoulder, 0.11f, WHITE);
        DrawSphere(elbow, 0.10f, ORANGE);
        DrawSphere(target, 0.11f, RED);
        EndMode3D();
        DrawText("Move Red IK Target: WASD (X/Y) + Q/E (Z)", 20, 20, 18, RAYWHITE);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
