// Lesson 9b: Custom TTF Fonts & 4-Octave Fractal-Noise Distressed Alpha-Mask Shader
#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include <math.h>
#include <stdlib.h>

static const char *DISTRESS_FS =
    "#version 330\n"
    "in vec2 fragTexCoord;\n"
    "in vec4 fragColor;\n"
    "uniform sampler2D texture0;\n"
    "uniform sampler2D gritTex;\n"
    "uniform float screenH;\n"
    "out vec4 finalColor;\n"
    "void main() {\n"
    "    vec4 c = texture(texture0, fragTexCoord) * fragColor;\n"
    "    vec2 gUV = vec2(gl_FragCoord.x / 600.0, (screenH - gl_FragCoord.y) / 300.0);\n"
    "    finalColor = vec4(c.rgb, c.a * texture(gritTex, gUV).a);\n"
    "}\n";

static float hash2(int ix, int iy, unsigned int seed) {
    unsigned int n = (unsigned int)ix * 374761393u + (unsigned int)iy * 668265263u + seed * 1442695041u;
    n = (n ^ (n >> 13)) * 1274126177u;
    return (float)((n ^ (n >> 16)) & 0x00ffffffu) / (float)0x01000000u;
}

static float vnoise(float x, float y, unsigned int seed) {
    int ix = (int)floorf(x), iy = (int)floorf(y);
    float fx = x - ix, fy = y - iy;
    float ux = fx * fx * (3.0f - 2.0f * fx), uy = fy * fy * (3.0f - 2.0f * fy);
    float a = hash2(ix, iy, seed), b = hash2(ix + 1, iy, seed), c = hash2(ix, iy + 1, seed), d = hash2(ix + 1, iy + 1, seed);
    return (a + (b - a) * ux) + ((c + (d - c) * ux) - (a + (b - a) * ux)) * uy;
}

int main(void) {
    InitWindow(960, 540, "Ex 09b: Distressed Fractal-Noise Font Shader");
    SetTargetFPS(60);

    const char *antonPath = FileExists("assets/fonts/Anton-Regular.ttf")
        ? "assets/fonts/Anton-Regular.ttf" : "../assets/fonts/Anton-Regular.ttf";
    Font anton = FileExists(antonPath) ? LoadFontEx(antonPath, 144, NULL, 0) : GetFontDefault();
    SetTextureFilter(anton.texture, TEXTURE_FILTER_BILINEAR);

    const int W = 600, H = 300;
    Color *px = (Color *)MemAlloc((unsigned int)(W * H) * sizeof(Color));
    for (int y = 0; y < H; y++) {
        for (int x = 0; x < W; x++) {
            float freq = 0.03f, amp = 0.5f, sum = 0.0f, norm = 0.0f;
            for (int o = 0; o < 4; o++) {
                sum += amp * vnoise((float)x * freq, (float)y * freq, 4u + (unsigned int)o * 31u);
                norm += amp; freq *= 2.0f; amp *= 0.5f;
            }
            float a = Clamp(-9.0f * (sum / norm) + 6.58f, 0.0f, 1.0f);
            px[y * W + x] = (Color){ 255, 255, 255, (unsigned char)(a * 255.0f) };
        }
    }
    Texture2D grit = LoadTextureFromImage((Image){ px, W, H, 1, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8 });
    SetTextureWrap(grit, TEXTURE_WRAP_REPEAT);
    MemFree(px);

    Shader sh = LoadShaderFromMemory(NULL, DISTRESS_FS);
    int locGrit = GetShaderLocation(sh, "gritTex");
    int locH = GetShaderLocation(sh, "screenH");

    Camera3D cam = { .position = { 4, 3, 6 }, .target = { 0, 0, 0 }, .up = { 0, 1, 0 }, .fovy = 45, .projection = CAMERA_PERSPECTIVE };
    float t = 0.0f;

    while (!WindowShouldClose()) {
        t += GetFrameTime();
        cam.position.x = cosf(t * 0.6f) * 6.0f;
        cam.position.z = sinf(t * 0.6f) * 6.0f;

        BeginDrawing();
        ClearBackground((Color){ 8, 10, 15, 255 });
        BeginMode3D(cam);
        DrawGrid(12, 1.0f);
        DrawCube((Vector3){ 0, 1, 0 }, 2, 2, 2, (Color){ 80, 65, 55, 255 });
        EndMode3D();

        rlDrawRenderBatchActive();
        float shH = (float)GetScreenHeight();
        BeginShaderMode(sh);
        SetShaderValue(sh, locH, &shH, SHADER_UNIFORM_FLOAT);
        SetShaderValueTexture(sh, locGrit, grit);
        DrawTextEx(anton, "ALL HOPE", (Vector2){ 160, 184 }, 144, 6, BLACK);
        DrawTextEx(anton, "ALL HOPE", (Vector2){ 160, 180 }, 144, 6, (Color){ 214, 69, 60, 255 });
        rlDrawRenderBatchActive();
        EndShaderMode();

        DrawText("4-octave fractal noise carves transparent chips over live 3D", 20, 20, 18, RAYWHITE);
        EndDrawing();
    }

    UnloadShader(sh);
    UnloadTexture(grit);
    if (anton.texture.id != GetFontDefault().texture.id) UnloadFont(anton);
    CloseWindow();
    return 0;
}
