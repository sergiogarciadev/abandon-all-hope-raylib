#include "upgrades.h"
#include "core/state.h"
#include "core/math_util.h"
#include "audio/sfx.h"
#include "raylib.h"
#include <limits.h>
#include <math.h>
#include <stddef.h>

typedef enum {
    UPG_FIREPOWER = 0,
    UPG_HANDLING,
    UPG_SURVIVAL,
    UPG_FORTUNE
} UpgCategory;

typedef struct {
    const char *id;
    UpgCategory cat;
    const char *name;
    const char *desc;
    int maxLevel;
    uint8_t icon[8]; // 8x8 1-bit pixel art
} UpgradeDef;

static const UpgradeDef UPGRADE_DEFS[UPGRADE_COUNT] = {
    {
        "mag", UPG_HANDLING, "EXTENDED MAG", "+4 rounds per magazine", 5,
        { 0x3c, 0x24, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x7e }
    },
    {
        "reload", UPG_HANDLING, "QUICK HANDS", "reload 25% faster", 4,
        { 0x18, 0x3c, 0x7e, 0x18, 0x18, 0x7e, 0x3c, 0x18 }
    },
    {
        "damage", UPG_FIREPOWER, "HOLLOW POINTS", "+50% bullet damage", 6,
        { 0x18, 0x3c, 0x3c, 0x7e, 0x7e, 0x7e, 0x7e, 0x3c }
    },
    {
        "rate", UPG_FIREPOWER, "HAIR TRIGGER", "fire 20% faster", 4,
        { 0x18, 0x30, 0x7e, 0x3c, 0x18, 0x30, 0x60, 0x40 }
    },
    {
        "pierce", UPG_FIREPOWER, "PIERCING", "shots pierce +1 zombie", 3,
        { 0x08, 0x1c, 0x3e, 0x7f, 0x3e, 0x1c, 0x08, 0x08 }
    },
    {
        "spread", UPG_FIREPOWER, "BUCKSHOT", "+1 bullet per shot", 3,
        { 0x81, 0x42, 0x24, 0x18, 0x18, 0x24, 0x42, 0x81 }
    },
    {
        "hp", UPG_SURVIVAL, "THICK SKIN", "+1 max health, and heal it", 3,
        { 0x66, 0xff, 0xff, 0xff, 0x7e, 0x3c, 0x18, 0x00 }
    },
    {
        "speed", UPG_SURVIVAL, "FLEET FOOT", "move 12% faster", 3,
        { 0x1c, 0x3e, 0x7c, 0x38, 0x70, 0xe0, 0x40, 0x00 }
    },
    {
        "luck", UPG_FORTUNE, "SCAVENGER", "more supply drops", 3,
        { 0x3c, 0x7e, 0xdb, 0xff, 0xff, 0x7e, 0x3c, 0x00 }
    },
    {
        "heal", UPG_SURVIVAL, "PATCH UP", "restore all health", INT_MAX,
        { 0x18, 0x18, 0x7e, 0x7e, 0x18, 0x18, 0x00, 0x00 }
    }
};

static const uint32_t CAT_COLORS[4] = {
    0xd6453c, // UPG_FIREPOWER
    0xe8d28a, // UPG_HANDLING
    0x8fae6b, // UPG_SURVIVAL
    0x6fa8dc  // UPG_FORTUNE
};

static const char *CAT_NAMES[4] = {
    "FIREPOWER", "HANDLING", "SURVIVAL", "FORTUNE"
};

static int sChoices[3] = { -1, -1, -1 };
static int sChoiceCount = 0;
static int sHovered = -1;
static float sAnimT = 0.0f;

static bool upgrade_eligible(int idx) {
    if (idx < 0 || idx >= UPGRADE_COUNT) return false;
    if (gState.upgrades[idx] >= UPGRADE_DEFS[idx].maxLevel) return false;
    if (idx == 9) return gState.hp < gState.maxHp; // "heal"
    return true;
}

static void apply_upgrade(int idx) {
    gState.upgrades[idx]++;
    switch (idx) {
        case 0: gState.magSize += 4; gState.mag = gState.magSize; break; // mag
        case 1: gState.reloadTime *= 0.75f; break;                      // reload
        case 2: gState.damage += 0.5f; break;                           // damage
        case 3: gState.fireRate *= 0.8f; break;                         // rate
        case 4: gState.pierce++; break;                                 // pierce
        case 5: gState.spread++; break;                                 // spread
        case 6: gState.maxHp++; gState.hp++; break;                     // hp
        case 7: gState.speed *= 1.12f; break;                           // speed
        case 8: gState.dropBonus += 0.03f; break;                       // luck
        case 9: gState.hp = gState.maxHp; break;                        // heal
    }
}

void upgrades_open(void) {
    int pool[UPGRADE_COUNT];
    int poolN = 0;
    for (int i = 0; i < UPGRADE_COUNT; i++) {
        if (upgrade_eligible(i)) pool[poolN++] = i;
    }

    for (int i = poolN - 1; i > 0; i--) {
        int j = (int)(rnd01() * (float)(i + 1));
        if (j > i) j = i;
        int tmp = pool[i];
        pool[i] = pool[j];
        pool[j] = tmp;
    }

    sChoiceCount = poolN < 3 ? poolN : 3;
    for (int i = 0; i < sChoiceCount; i++) sChoices[i] = pool[i];

    sHovered = -1;
    sAnimT = 0.0f;
    gState.choosing = (sChoiceCount > 0);
    if (gState.choosing) sfx_deal(sChoiceCount);
}

void upgrades_close(void) {
    gState.choosing = false;
    sChoiceCount = 0;
    sHovered = -1;
}

static void card_rect(int i, int count, int sw, int sh, Rectangle *out) {
    float cw = 230.0f, ch = 290.0f, gap = 28.0f;
    float totalW = (float)count * cw + (float)(count - 1) * gap;
    float x0 = ((float)sw - totalW) * 0.5f;
    float y0 = ((float)sh - ch) * 0.5f + 22.0f;
    out->x = x0 + (float)i * (cw + gap);
    out->y = y0;
    out->width = cw;
    out->height = ch;
}

bool upgrades_update(float dt) {
    if (!gState.choosing) return false;
    sAnimT += dt;

    int sw = GetScreenWidth();
    int sh = GetScreenHeight();
    Vector2 mp = GetMousePosition();

    int newHover = -1;
    for (int i = 0; i < sChoiceCount; i++) {
        Rectangle r;
        card_rect(i, sChoiceCount, sw, sh, &r);
        if (CheckCollisionPointRec(mp, r)) {
            newHover = i;
            break;
        }
    }
    if (newHover != sHovered && newHover >= 0) {
        sfx_hover(newHover);
    }
    sHovered = newHover;

    int pickedSlot = -1;
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && sHovered >= 0) {
        pickedSlot = sHovered;
    }
    if (IsKeyPressed(KEY_ONE) && sChoiceCount >= 1) pickedSlot = 0;
    if (IsKeyPressed(KEY_TWO) && sChoiceCount >= 2) pickedSlot = 1;
    if (IsKeyPressed(KEY_THREE) && sChoiceCount >= 3) pickedSlot = 2;

    if (pickedSlot >= 0) {
        int idx = sChoices[pickedSlot];
        apply_upgrade(idx);
        sfx_upgrade((int)UPGRADE_DEFS[idx].cat);
        upgrades_close();
        return true;
    }
    return false;
}

static void draw_pixel_icon(const uint8_t rows[8], int cx, int cy, int pxSize, Color col) {
    int x0 = cx - 4 * pxSize;
    int y0 = cy - 4 * pxSize;
    for (int r = 0; r < 8; r++) {
        uint8_t row = rows[r];
        for (int c = 0; c < 8; c++) {
            if (row & (0x80 >> c)) {
                DrawRectangle(x0 + c * pxSize, y0 + r * pxSize, pxSize, pxSize, col);
            }
        }
    }
}

void upgrades_draw(void) {
    if (!gState.choosing) return;
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();

    DrawRectangle(0, 0, sw, sh, (Color){ 6, 8, 12, 185 });

    const char *hdr = "WAVE CLEARED - CHOOSE AN UPGRADE";
    int hw = MeasureText(hdr, 28);
    DrawText(hdr, (sw - hw) / 2, sh / 2 - 205, 28, hex_color(0xe8d28a, 255));

    const char *sub = "Click a card or press [1] [2] [3]";
    int suw = MeasureText(sub, 16);
    DrawText(sub, (sw - suw) / 2, sh / 2 - 168, 16, (Color){ 175, 185, 200, 220 });

    for (int i = 0; i < sChoiceCount; i++) {
        int idx = sChoices[i];
        const UpgradeDef *u = &UPGRADE_DEFS[idx];
        Color catCol = hex_color(CAT_COLORS[u->cat], 255);

        Rectangle r;
        card_rect(i, sChoiceCount, sw, sh, &r);

        float deal = clampf((sAnimT - (float)i * 0.08f) / 0.25f, 0.0f, 1.0f);
        float ease = ease_smooth(deal);
        r.y += (1.0f - ease) * 36.0f;
        if (i == sHovered) r.y -= 8.0f;

        Color bg = (i == sHovered) ? (Color){ 28, 32, 42, 245 } : (Color){ 18, 21, 28, 238 };
        Color border = (i == sHovered) ? catCol : (Color){ 75, 82, 98, 210 };

        DrawRectangleRounded(r, 0.06f, 8, bg);
        DrawRectangleRoundedLinesEx(r, 0.06f, 8, i == sHovered ? 2.5f : 1.5f, border);

        // Category label
        const char *catName = CAT_NAMES[u->cat];
        int cw = MeasureText(catName, 13);
        DrawText(catName, (int)(r.x + (r.width - (float)cw) * 0.5f), (int)r.y + 18, 13, catCol);

        // 8x8 pixel icon
        draw_pixel_icon(u->icon, (int)(r.x + r.width * 0.5f), (int)r.y + 84, 7, catCol);

        // Upgrade name
        int nw = MeasureText(u->name, 20);
        DrawText(u->name, (int)(r.x + (r.width - (float)nw) * 0.5f), (int)r.y + 144, 20, RAYWHITE);

        // Description
        int dw = MeasureText(u->desc, 15);
        DrawText(u->desc, (int)(r.x + (r.width - (float)dw) * 0.5f), (int)r.y + 178, 15, (Color){ 185, 195, 210, 255 });

        // Rank info / key hint
        const char *rank = (u->maxLevel == INT_MAX)
            ? "CONSUMABLE"
            : TextFormat("RANK %d / %d", gState.upgrades[idx] + 1, u->maxLevel);
        int rw = MeasureText(rank, 13);
        DrawText(rank, (int)(r.x + (r.width - (float)rw) * 0.5f), (int)r.y + 225, 13, (Color){ 140, 150, 168, 220 });

        const char *key = TextFormat("[%d]", i + 1);
        int kw = MeasureText(key, 16);
        DrawText(key, (int)(r.x + (r.width - (float)kw) * 0.5f), (int)r.y + 252, 16, catCol);
    }
}
