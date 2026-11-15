#include "raylib.h"
#include "raymath.h"
#include "core/math_util.h"
#include "core/scene.h"

static Texture2D make_ground_texture(void) {
    Image img = GenImageColor(128, 128, hex_color(0x1b1e15, 255));
    SeededRng rng = rng_seed(5);
    const uint32_t tones[6] = { 0x232b19, 0x161812, 0x2a2418, 0x1e2716, 0x121410, 0x2c3320 };
    for (int i = 0; i < 2600; i++) {
        Color c = hex_color(tones[(int)(rng_next(&rng) * 6.0f) % 6], 255);
        int s = rng_next(&rng) < 0.85f ? 2 : 4;
        int x = ((int)(rng_next(&rng) * 64.0f)) * 2;
        int y = ((int)(rng_next(&rng) * 64.0f)) * 2;
        ImageDrawRectangle(&img, x, y, s, s, c);
    }
    Texture2D tex = LoadTextureFromImage(img);
    UnloadImage(img);
    SetTextureWrap(tex, TEXTURE_WRAP_REPEAT);
    SetTextureFilter(tex, TEXTURE_FILTER_POINT);
    return tex;
}

int main(void) {
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    InitWindow(1280, 720, "Abandon All Hope");
    SetTargetFPS(60);

    scene_init();

    Texture2D groundTex = make_ground_texture();
    Mesh groundMesh = GenMeshPlane(200.0f, 200.0f, 1, 1);
    // Tile UVs 25x25 across the 200x200 plane
    for (int i = 0; i < groundMesh.vertexCount; i++) {
        groundMesh.texcoords[i * 2 + 0] *= 25.0f;
        groundMesh.texcoords[i * 2 + 1] *= 25.0f;
    }
    UpdateMeshBuffer(groundMesh, 1, groundMesh.texcoords, groundMesh.vertexCount * 2 * (int)sizeof(float), 0);

    Model groundModel = LoadModelFromMesh(groundMesh);
    groundModel.materials[0].shader = gScene.litShader;
    groundModel.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = groundTex;

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

        gScene.camera.target = playerPos;
        gScene.camera.position = (Vector3){
            playerPos.x + CAM_OFFSET_X,
            playerPos.y + CAM_OFFSET_Y,
            playerPos.z + CAM_OFFSET_Z
        };

        scene_update_lighting((Vector3){ 2.26f, 1.7f, 11.3f }, 0.0f, (Vector3){0}, 0.0f, WHITE);

        BeginDrawing();
        ClearBackground(gScene.fogColor);

        BeginMode3D(gScene.camera);
        int useTex = 1;
        SetShaderValue(gScene.litShader, gScene.locUseTex, &useTex, SHADER_UNIFORM_INT);
        DrawModel(groundModel, (Vector3){ 0.0f, -0.06f, 0.0f }, 1.0f, WHITE);
        useTex = 0;
        SetShaderValue(gScene.litShader, gScene.locUseTex, &useTex, SHADER_UNIFORM_INT);

        // Placeholder survivor marker
        DrawCube((Vector3){ playerPos.x, 0.9f, playerPos.z }, 0.6f, 1.8f, 0.4f, hex_color(0xe0662a, 255));
        EndMode3D();

        DrawText("Step 2: Empty World (WASD to move)", 20, 20, 20, RAYWHITE);
        EndDrawing();
    }

    UnloadModel(groundModel);
    UnloadTexture(groundTex);
    scene_cleanup();
    CloseWindow();
    return 0;
}
