// Lesson 9a: 8x7 Pixel-Art Heart HP Pips & 2-Tone (X/o) 8x8 Upgrade Cards
#include "raylib.h"

static const char *HEART[7] = {
    ".XX..XX.",
    "XXXXXXXX",
    "XXXXXXXX",
    "XXXXXXXX",
    ".XXXXXX.",
    "..XXXX..",
    "...XX..."
};

static const char *ICONS[3][8] = {
    { "XX.XX.XX", "XX.XX.XX", "oo.oo.oo", "oo.oo.oo", "oo.oo.oo", "oo.oo.oo", "XX.XX.XX", "........" },
    { ".o..o...", ".o..o.X.", ".o..o..X", "XXXXXXXX", ".o..o..X", ".o..o.X.", ".o..o...", "........" },
    { "XXXXXXXX", "XooooooX", "XoooXooX", "XooXXXoX", ".XooXoX.", ".XooooX.", "..XooX..", "...XX..." }
};

static void draw_grid_icon(const char *const rows[], int rowCount, int cx, int cy, int px, Color col) {
    int x0 = cx - 4 * px, y0 = cy - (rowCount * px) / 2;
    Color dim = { col.r, col.g, col.b, 95 };
    for (int r = 0; r < rowCount; r++) {
        for (int c = 0; c < 8; c++) {
            if (rows[r][c] == 'X') DrawRectangle(x0 + c * px, y0 + r * px, px, px, col);
            else if (rows[r][c] == 'o') DrawRectangle(x0 + c * px, y0 + r * px, px, px, dim);
        }
    }
}

int main(void) {
    InitWindow(960, 540, "Ex 09a: Pixel-Art Hearts & 2-Tone Upgrade Cards");
    SetTargetFPS(60);

    int hp = 2, maxHp = 4;
    const char *names[3] = { "EXTENDED MAG", "PIERCING", "THICK SKIN" };
    Color cols[3] = { { 232, 210, 138, 255 }, { 214, 69, 60, 255 }, { 143, 174, 107, 255 } };

    while (!WindowShouldClose()) {
        if (IsKeyPressed(KEY_UP) && hp < maxHp) hp++;
        if (IsKeyPressed(KEY_DOWN) && hp > 0) hp--;

        BeginDrawing();
        ClearBackground((Color){ 10, 12, 16, 255 });

        for (int i = 0; i < maxHp; i++) {
            Color c = (i < hp) ? (Color){ 214, 69, 60, 255 } : (Color){ 45, 48, 56, 190 };
            draw_grid_icon(HEART, 7, 420 + i * 40, 60, 4, c);
        }

        for (int i = 0; i < 3; i++) {
            Rectangle r = { 135.0f + i * 240.0f, 150.0f, 210.0f, 260.0f };
            bool hover = CheckCollisionPointRec(GetMousePosition(), r);
            if (hover) r.y -= 8.0f;
            DrawRectangleRounded(r, 0.06f, 8, hover ? (Color){ 28, 32, 42, 255 } : (Color){ 18, 21, 28, 255 });
            DrawRectangleRoundedLinesEx(r, 0.06f, 8, hover ? 2.5f : 1.5f, hover ? cols[i] : DARKGRAY);
            draw_grid_icon(ICONS[i], 8, (int)(r.x + r.width * 0.5f), (int)r.y + 90, 7, cols[i]);
            int tw = MeasureText(names[i], 18);
            DrawText(names[i], (int)(r.x + (r.width - tw) * 0.5f), (int)r.y + 165, 18, RAYWHITE);
        }

        DrawText("UP/DOWN to change HP hearts | Hover cards", 20, 480, 18, LIGHTGRAY);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
