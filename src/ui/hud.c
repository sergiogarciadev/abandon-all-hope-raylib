#include "hud.h"
#include "core/state.h"
#include "core/storage.h"
#include "core/math_util.h"
#include "entities/zombies.h"
#include "raylib.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

static char sToastMsg[96] = "";
static float sToastT = 0.0f;

static char sBannerTitle[64] = "";
static char sBannerSub[64] = "";
static float sBannerT = 0.0f;

static float sFlashT = 0.0f;

void hud_init(void) {
    hud_reset();
}

void hud_reset(void) {
    sToastT = 0.0f;
    sBannerT = 0.0f;
    sFlashT = 0.0f;
    sToastMsg[0] = '\0';
    sBannerTitle[0] = '\0';
    sBannerSub[0] = '\0';
}

void hud_toast(const char *msg) {
    if (!msg) return;
    snprintf(sToastMsg, sizeof(sToastMsg), "%s", msg);
    sToastT = 1.6f;
}

void hud_wave_banner(const char *title, const char *sub) {
    snprintf(sBannerTitle, sizeof(sBannerTitle), "%s", title ? title : "");
    snprintf(sBannerSub, sizeof(sBannerSub), "%s", sub ? sub : "");
    sBannerT = 2.6f;
}

void hud_damage_flash(void) {
    sFlashT = 0.55f;
}

void hud_update(float dt) {
    if (sToastT > 0.0f) sToastT = fmaxf(0.0f, sToastT - dt);
    if (sBannerT > 0.0f) sBannerT = fmaxf(0.0f, sBannerT - dt);
    if (sFlashT > 0.0f) sFlashT = fmaxf(0.0f, sFlashT - dt);
}

void hud_draw(void) {
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();

    // Red damage vignette flash
    if (sFlashT > 0.0f) {
        float a = (sFlashT / 0.55f);
        uint8_t alpha = (uint8_t)(a * a * 125.0f);
        Color edge = { 185, 22, 18, alpha };
        int border = 48;
        DrawRectangleGradientV(0, 0, sw, border, edge, BLANK);
        DrawRectangleGradientV(0, sh - border, sw, border, BLANK, edge);
        DrawRectangleGradientH(0, 0, border, sh, edge, BLANK);
        DrawRectangleGradientH(sw - border, 0, border, sh, BLANK, edge);
    }

    // Top-left: Wave / Kills / Left
    int left = gState.toSpawn + gZombieCount;
    DrawRectangle(18, 16, 210, 82, (Color){ 10, 12, 16, 150 });
    DrawRectangleLines(18, 16, 210, 82, (Color){ 80, 90, 105, 130 });
    DrawText(TextFormat("WAVE %d", gState.wave), 30, 24, 24, hex_color(0xe8d28a, 255));
    DrawText(TextFormat("KILLS: %d", gState.kills), 30, 53, 18, hex_color(0xd8dde6, 255));
    DrawText(TextFormat("LEFT: %d", left), 30, 74, 18, hex_color(0xd6453c, 255));

    // Top-center: Health pips + active powerups
    int pipW = 24, pipH = 14, pipGap = 6;
    int totalHpW = gState.maxHp * pipW + (gState.maxHp - 1) * pipGap;
    int hpX = (sw - totalHpW) / 2;
    int hpY = 22;
    DrawRectangle(hpX - 12, hpY - 8, totalHpW + 24, pipH + 16, (Color){ 10, 12, 16, 150 });
    for (int i = 0; i < gState.maxHp; i++) {
        int x = hpX + i * (pipW + pipGap);
        Color fill = (i < gState.hp) ? hex_color(0xd6453c, 255) : (Color){ 42, 46, 54, 200 };
        DrawRectangle(x, hpY, pipW, pipH, fill);
        DrawRectangleLines(x, hpY, pipW, pipH, (Color){ 20, 22, 26, 220 });
    }

    int barY = hpY + pipH + 14;
    if (gState.pDamage > 0.0f) {
        const char *txt = TextFormat("2X DAMAGE  %.1fs", gState.pDamage);
        int tw = MeasureText(txt, 16);
        DrawRectangle((sw - tw) / 2 - 10, barY - 3, tw + 20, 22, (Color){ 10, 12, 16, 160 });
        DrawText(txt, (sw - tw) / 2, barY, 16, hex_color(0xff8c32, 255));
        barY += 26;
    }
    if (gState.pAmmo > 0.0f) {
        const char *txt = TextFormat("INF AMMO  %.1fs", gState.pAmmo);
        int tw = MeasureText(txt, 16);
        DrawRectangle((sw - tw) / 2 - 10, barY - 3, tw + 20, 22, (Color){ 10, 12, 16, 160 });
        DrawText(txt, (sw - tw) / 2, barY, 16, hex_color(0x55d0ff, 255));
    }

    // Bottom-center: Magazine & Reload bar
    int ammoY = sh - 54;
    if (gState.reloading) {
        int bw = 200, bh = 16;
        int bx = (sw - bw) / 2;
        float u = clampf(gState.reloadT / gState.reloadTime, 0.0f, 1.0f);
        DrawRectangle(bx - 4, ammoY - 4, bw + 8, bh + 8, (Color){ 10, 12, 16, 180 });
        DrawRectangle(bx, ammoY, (int)(bw * u), bh, hex_color(0xe8d28a, 255));
        DrawRectangleLines(bx, ammoY, bw, bh, (Color){ 180, 190, 200, 180 });
        const char *rtxt = "RELOADING...";
        int rtw = MeasureText(rtxt, 16);
        DrawText(rtxt, (sw - rtw) / 2, ammoY - 24, 16, hex_color(0xe8d28a, 255));
    } else if (gState.pAmmo > 0.0f) {
        const char *atxt = "AMMO: INF";
        int tw = MeasureText(atxt, 22);
        DrawRectangle((sw - tw) / 2 - 14, ammoY - 6, tw + 28, 34, (Color){ 10, 12, 16, 160 });
        DrawText(atxt, (sw - tw) / 2, ammoY, 22, hex_color(0x55d0ff, 255));
    } else {
        int maxShow = gState.magSize <= 24 ? gState.magSize : 24;
        int rw = 7, rg = 4;
        int rowW = maxShow * rw + (maxShow - 1) * rg;
        int rx0 = (sw - rowW) / 2;
        DrawRectangle(rx0 - 12, ammoY - 8, rowW + 24, 30, (Color){ 10, 12, 16, 160 });
        for (int i = 0; i < maxShow; i++) {
            bool full = i < (gState.mag * maxShow + gState.magSize - 1) / gState.magSize;
            if (gState.magSize <= 24) full = i < gState.mag;
            Color c = full ? hex_color(0xe8d28a, 255) : (Color){ 55, 58, 66, 180 };
            DrawRectangle(rx0 + i * (rw + rg), ammoY, rw, 14, c);
        }
        const char *cnt = TextFormat("%d / %d", gState.mag, gState.magSize);
        int ctw = MeasureText(cnt, 16);
        DrawText(cnt, (sw - ctw) / 2, ammoY - 24, 16, hex_color(0xd8dde6, 255));

        if (gState.mag <= 3) {
            const char *hint = "[R] RELOAD";
            int htw = MeasureText(hint, 16);
            DrawText(hint, (sw - htw) / 2, ammoY - 46, 16, hex_color(0xff8c32, 255));
        }
    }

    // Bottom-right: Mute & Pause hints
    const char *hintRight = gSettings.muted ? "[M] MUTED   [ESC] PAUSE" : "[M] MUTE   [ESC] PAUSE";
    int hrw = MeasureText(hintRight, 15);
    DrawText(hintRight, sw - hrw - 20, sh - 28, 15,
             gSettings.muted ? hex_color(0xd6453c, 220) : (Color){ 170, 180, 195, 190 });

    // Wave Banner (top-center)
    if (sBannerT > 0.0f) {
        float fade = clampf(sBannerT / 0.5f, 0.0f, 1.0f);
        if (sBannerT > 2.2f) fade = clampf((2.6f - sBannerT) / 0.4f, 0.0f, 1.0f);
        uint8_t a = (uint8_t)(fade * 255.0f);
        int bw = MeasureText(sBannerTitle, 34);
        int sw2 = MeasureText(sBannerSub, 18);
        int y = 94;
        DrawText(sBannerTitle, (sw - bw) / 2, y, 34, (Color){ 232, 210, 138, a });
        if (sBannerSub[0] != '\0') {
            DrawText(sBannerSub, (sw - sw2) / 2, y + 40, 18, (Color){ 195, 205, 220, a });
        }
    }

    // Toast (above ammo)
    if (sToastT > 0.0f) {
        float fade = clampf(sToastT / 0.4f, 0.0f, 1.0f);
        uint8_t a = (uint8_t)(fade * 255.0f);
        int tw = MeasureText(sToastMsg, 20);
        DrawRectangle((sw - tw) / 2 - 14, sh - 122, tw + 28, 32, (Color){ 12, 14, 18, (uint8_t)(fade * 190.0f) });
        DrawText(sToastMsg, (sw - tw) / 2, sh - 116, 20, (Color){ 240, 244, 248, a });
    }
}
