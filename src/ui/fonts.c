#include "fonts.h"
#include "core/math_util.h"
#include <stddef.h>

Font gFontAnton = {0};
Font gFontJersey = {0};
Font gFontNightshade = {0};

static bool sAntonLoaded = false;
static bool sJerseyLoaded = false;
static bool sNightshadeLoaded = false;

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
                                 "../assets/fonts/Anton-Regular.ttf", 96, &sAntonLoaded);
    gFontJersey = load_font_asset("assets/fonts/Jersey25-Regular.ttf",
                                  "../assets/fonts/Jersey25-Regular.ttf", 36, &sJerseyLoaded);
    gFontNightshade = load_font_asset("assets/fonts/JimNightshade-Regular.ttf",
                                      "../assets/fonts/JimNightshade-Regular.ttf", 80, &sNightshadeLoaded);
}

void fonts_cleanup(void) {
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
    if (!text) return;
    Color glow = { color.r, color.g, color.b, (uint8_t)(color.a / 5) };
    DrawTextEx(font, text, (Vector2){ x - 1.0f, y + 1.0f }, size, spacing, glow);
    DrawTextEx(font, text, (Vector2){ x + 1.0f, y + 1.0f }, size, spacing, glow);
    DrawTextEx(font, text, (Vector2){ x, y + 4.0f }, size, spacing, (Color){ 0, 0, 0, color.a });
    DrawTextEx(font, text, (Vector2){ x, y }, size, spacing, color);

    // Subtle chipped-paint grit speckles over the title glyphs
    float w = font_measure(font, text, size, spacing);
    SeededRng rng = { seed ? seed : 42u };
    int specks = (int)(w * 0.25f);
    for (int i = 0; i < specks; i++) {
        float sx = x + rng_next(&rng) * w;
        float sy = y + size * (0.14f + rng_next(&rng) * 0.72f);
        float sw = 1.0f + rng_next(&rng) * 2.5f;
        float sh = 1.0f + rng_next(&rng) * 2.0f;
        DrawRectangle((int)sx, (int)sy, (int)sw, (int)sh, (Color){ 8, 9, 13, 145 });
    }
}
