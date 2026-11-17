#include "fonts.h"
#include "core/math_util.h"
#include "rlgl.h"
#include <math.h>
#include <stddef.h>
#include <stdlib.h>

Font gFontAnton = {0};
Font gFontJersey = {0};
Font gFontNightshade = {0};

static bool sAntonLoaded = false;
static bool sJerseyLoaded = false;
static bool sNightshadeLoaded = false;

static Texture2D sGritTex = {0};
static Shader sDistressShader = {0};
static int sGritLoc = -1;
static int sScreenHLoc = -1;

#if defined(PLATFORM_WEB)
static const char *DISTRESS_FS =
    "#version 100\n"
    "precision mediump float;\n"
    "varying vec2 fragTexCoord;\n"
    "varying vec4 fragColor;\n"
    "uniform sampler2D texture0;\n"
    "uniform sampler2D gritTex;\n"
    "uniform float screenH;\n"
    "void main() {\n"
    "    vec4 c = texture2D(texture0, fragTexCoord) * fragColor;\n"
    "    vec2 gUV = vec2(gl_FragCoord.x / 512.0, (screenH - gl_FragCoord.y) / 256.0);\n"
    "    float mask = texture2D(gritTex, gUV).a;\n"
    "    gl_FragColor = vec4(c.rgb, c.a * mask);\n"
    "}\n";
#else
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
    "    vec2 gUV = vec2(gl_FragCoord.x / 512.0, (screenH - gl_FragCoord.y) / 256.0);\n"
    "    float mask = texture(gritTex, gUV).a;\n"
    "    finalColor = vec4(c.rgb, c.a * mask);\n"
    "}\n";
#endif

static float hash2d(int ix, int iy, unsigned int seed) {
    unsigned int n = (unsigned int)ix * 374761393u + (unsigned int)iy * 668265263u + seed * 1442695041u;
    n = (n ^ (n >> 13)) * 1274126177u;
    return (float)((n ^ (n >> 16)) & 0x00ffffffu) / (float)0x01000000u;
}

static float value_noise_2d(float x, float y, unsigned int seed) {
    int ix = (int)floorf(x);
    int iy = (int)floorf(y);
    float fx = x - (float)ix;
    float fy = y - (float)iy;
    float ux = fx * fx * (3.0f - 2.0f * fx);
    float uy = fy * fy * (3.0f - 2.0f * fy);
    float a = hash2d(ix, iy, seed);
    float b = hash2d(ix + 1, iy, seed);
    float c = hash2d(ix, iy + 1, seed);
    float d = hash2d(ix + 1, iy + 1, seed);
    return lerpf(lerpf(a, b, ux), lerpf(c, d, ux), uy);
}

static void build_grit_mask(void) {
    const int W = 512, H = 256; // Power-of-two for WebGL1 TEXTURE_WRAP_REPEAT support
    Color *px = (Color *)malloc((size_t)(W * H) * sizeof(Color));
    if (!px) return;

    // 4-octave fractal noise matching SVG feTurbulence(baseFrequency=0.03, numOctaves=4, seed=4)
    // and feColorMatrix alpha = clamp(-9 * n + 6.58, 0, 1)
    for (int y = 0; y < H; y++) {
        for (int x = 0; x < W; x++) {
            float freq = 0.03f;
            float amp = 0.5f;
            float sum = 0.0f;
            float norm = 0.0f;
            for (int oct = 0; oct < 4; oct++) {
                sum += amp * value_noise_2d((float)x * freq, (float)y * freq, 4u + (unsigned int)oct * 31u);
                norm += amp;
                freq *= 2.0f;
                amp *= 0.5f;
            }
            float n = sum / norm;
            float alpha = clampf(-9.0f * n + 6.58f, 0.0f, 1.0f);
            px[y * W + x] = (Color){ 255, 255, 255, (uint8_t)(alpha * 255.0f) };
        }
    }

    Image img = {
        .data = px,
        .width = W,
        .height = H,
        .mipmaps = 1,
        .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8
    };
    sGritTex = LoadTextureFromImage(img);
    SetTextureWrap(sGritTex, TEXTURE_WRAP_REPEAT);
    SetTextureFilter(sGritTex, TEXTURE_FILTER_BILINEAR);
    free(px);

    sDistressShader = LoadShaderFromMemory(NULL, DISTRESS_FS);
    sGritLoc = GetShaderLocation(sDistressShader, "gritTex");
    sScreenHLoc = GetShaderLocation(sDistressShader, "screenH");
}

static Font load_font_asset(const char *relPath, const char *altPath, int baseSize, bool *loadedOut) {
    const char *path = FileExists(relPath) ? relPath : (FileExists(altPath) ? altPath : NULL);
    if (path) {
        Font f = LoadFontEx(path, baseSize, NULL, 0);
        if (f.texture.id > 0) {
            SetTextureFilter(f.texture, TEXTURE_FILTER_BILINEAR);
            *loadedOut = true;
            return f;
        }
    }
    *loadedOut = false;
    return GetFontDefault();
}

void fonts_init(void) {
    gFontAnton = load_font_asset("assets/fonts/Anton-Regular.ttf",
                                 "../assets/fonts/Anton-Regular.ttf", 144, &sAntonLoaded);
    gFontJersey = load_font_asset("assets/fonts/Jersey25-Regular.ttf",
                                  "../assets/fonts/Jersey25-Regular.ttf", 36, &sJerseyLoaded);
    gFontNightshade = load_font_asset("assets/fonts/JimNightshade-Regular.ttf",
                                      "../assets/fonts/JimNightshade-Regular.ttf", 80, &sNightshadeLoaded);
    build_grit_mask();
}

void fonts_cleanup(void) {
    if (sGritTex.id > 0) {
        UnloadTexture(sGritTex);
        sGritTex.id = 0;
    }
    if (sDistressShader.id > 0) {
        UnloadShader(sDistressShader);
        sDistressShader.id = 0;
    }
    if (sAntonLoaded) UnloadFont(gFontAnton);
    if (sJerseyLoaded) UnloadFont(gFontJersey);
    if (sNightshadeLoaded) UnloadFont(gFontNightshade);
    sAntonLoaded = false;
    sJerseyLoaded = false;
    sNightshadeLoaded = false;
}

float font_measure(Font font, const char *text, float size, float spacing) {
    if (!text) return 0.0f;
    return MeasureTextEx(font, text, size, spacing).x;
}

void font_draw(Font font, const char *text, float x, float y, float size, float spacing, Color color) {
    if (!text) return;
    DrawTextEx(font, text, (Vector2){ x, y }, size, spacing, color);
}

void font_draw_shadow(Font font, const char *text, float x, float y, float size, float spacing,
                      Color color, Color shadow, float off) {
    if (!text) return;
    DrawTextEx(font, text, (Vector2){ x, y + off }, size, spacing, shadow);
    DrawTextEx(font, text, (Vector2){ x, y }, size, spacing, color);
}

void font_draw_distressed(Font font, const char *text, float x, float y, float size, float spacing,
                          Color color, unsigned int seed) {
    (void)seed;
    if (!text) return;

    if (sDistressShader.id > 0 && sGritTex.id > 0) {
        rlDrawRenderBatchActive();
        float sh = (float)GetScreenHeight();
        BeginShaderMode(sDistressShader);
        SetShaderValue(sDistressShader, sScreenHLoc, &sh, SHADER_UNIFORM_FLOAT);
        SetShaderValueTexture(sDistressShader, sGritLoc, sGritTex);

        Color glow = { color.r, color.g, color.b, (uint8_t)(color.a / 4) };
        DrawTextEx(font, text, (Vector2){ x - 1.5f, y + 1.5f }, size, spacing, glow);
        DrawTextEx(font, text, (Vector2){ x + 1.5f, y + 1.5f }, size, spacing, glow);
        DrawTextEx(font, text, (Vector2){ x, y + 4.0f }, size, spacing, (Color){ 0, 0, 0, color.a });
        DrawTextEx(font, text, (Vector2){ x, y }, size, spacing, color);

        rlDrawRenderBatchActive();
        EndShaderMode();
        return;
    }

    DrawTextEx(font, text, (Vector2){ x, y + 4.0f }, size, spacing, (Color){ 0, 0, 0, color.a });
    DrawTextEx(font, text, (Vector2){ x, y }, size, spacing, color);
}
