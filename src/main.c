#include "raylib.h"
#include "raymath.h"
#include "core/math_util.h"
#include "core/scene.h"
#include "world/world.h"
#include "world/openings.h"

int main(void) {
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    InitWindow(1280, 720, "Abandon All Hope");
    SetTargetFPS(60);

    scene_init();
    world_init();

    Vector3 playerPos = { 0.0f, 0.0f, 0.0f };

    while (!WindowShouldClose()) {
        float dt = fminf(GetFrameTime(), 0.05f);

        float mx = 0.0f, mz = 0.0f;
        if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) mz -= 1.0f;
        if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) mz += 1.0f;
        if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) mx -= 1.0f;
        if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) mx += 1.0f;
        float len = hypotf(mx, mz);
        if (len > 0.0f) {
            playerPos.x += (mx / len) * 6.2f * dt;
            playerPos.z += (mz / len) * 6.2f * dt;
        }
        float lim = WORLD_HALF - 0.7f;
        playerPos.x = clampf(playerPos.x, -lim, lim);
        playerPos.z = clampf(playerPos.z, -lim, lim);

        world_update(dt, 0.0f);
        update_openings(dt);

        gScene.camera.target = playerPos;
        gScene.camera.position = (Vector3){
            playerPos.x + CAM_OFFSET_X,
            playerPos.y + CAM_OFFSET_Y,
            playerPos.z + CAM_OFFSET_Z
        };

        scene_update_lighting(gWorld.lanternPos, gWorld.lanternIntensity, (Vector3){0}, 0.0f, WHITE);

        BeginDrawing();
        ClearBackground(gScene.fogColor);

        BeginMode3D(gScene.camera);
        world_draw(playerPos);
        DrawCube((Vector3){ playerPos.x, 0.9f, playerPos.z }, 0.6f, 1.8f, 0.4f, hex_color(0xe0662a, 255));
        EndMode3D();

        DrawText("Step 3: Cabin, Yard & Forest Models (WASD to move)", 20, 20, 20, RAYWHITE);
        EndDrawing();
    }

    world_cleanup();
    scene_cleanup();
    CloseWindow();
    return 0;
}
