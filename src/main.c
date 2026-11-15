#include "raylib.h"
#include "raymath.h"
#include "core/math_util.h"
#include "core/scene.h"
#include "core/state.h"
#include "world/world.h"
#include "world/openings.h"
#include "entities/player.h"

int main(void) {
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    InitWindow(1280, 720, "Abandon All Hope");
    SetTargetFPS(60);

    state_reset_stats();
    scene_init();
    world_init();
    player_init();

    while (!WindowShouldClose()) {
        float dt = fminf(GetFrameTime(), 0.05f);
        float px = gPlayer.position.x, pz = gPlayer.position.z;

        float mx = 0.0f, mz = 0.0f;
        if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) mz -= 1.0f;
        if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) mz += 1.0f;
        if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) mx -= 1.0f;
        if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) mx += 1.0f;
        float len = hypotf(mx, mz);
        if (len > 0.0f) {
            gPlayer.position.x += (mx / len) * gState.speed * dt;
            gPlayer.position.z += (mz / len) * gState.speed * dt;
        }
        float lim = WORLD_HALF - 0.7f;
        gPlayer.position.x = clampf(gPlayer.position.x, -lim, lim);
        gPlayer.position.z = clampf(gPlayer.position.z, -lim, lim);

        Ray ray = GetScreenToWorldRay(GetMousePosition(), gScene.camera);
        if (fabsf(ray.direction.y) > 1e-5f) {
            float t = (0.9f - ray.position.y) / ray.direction.y;
            if (t > 0.0f) {
                float ax = ray.position.x + ray.direction.x * t;
                float az = ray.position.z + ray.direction.z * t;
                gPlayer.rotY = atan2f(ax - gPlayer.position.x, az - gPlayer.position.z);
            }
        }

        float inv = dt > 0.0f ? 1.0f / dt : 0.0f;
        player_animate(dt, (gPlayer.position.x - px) * inv, (gPlayer.position.z - pz) * inv);

        world_update(dt, 0.0f);
        update_openings(dt);

        gScene.camera.target = gPlayer.position;
        gScene.camera.position = (Vector3){
            gPlayer.position.x + CAM_OFFSET_X,
            gPlayer.position.y + CAM_OFFSET_Y,
            gPlayer.position.z + CAM_OFFSET_Z
        };

        scene_update_lighting(gWorld.lanternPos, gWorld.lanternIntensity,
                              gPlayer.muzzleLightPos, gPlayer.muzzleLightInt, gPlayer.flashColor);

        BeginDrawing();
        ClearBackground(gScene.fogColor);

        BeginMode3D(gScene.camera);
        world_draw(gPlayer.position);
        player_draw();
        EndMode3D();

        DrawText("Step 4: Procedural Gait & Two-Bone IK Survivor Rig", 20, 20, 20, RAYWHITE);
        EndDrawing();
    }

    player_cleanup();
    world_cleanup();
    scene_cleanup();
    CloseWindow();
    return 0;
}
