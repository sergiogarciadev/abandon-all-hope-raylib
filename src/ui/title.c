#include "title.h"
#include "fonts.h"
#include "core/storage.h"
#include "core/scene.h"
#include "core/math_util.h"
#include "audio/sfx.h"
#include <math.h>
#include <stdio.h>

ScreenMode gScreen = SCREEN_TITLE;
float gSwoopT = -1.0f;

static TitleCallbacks sCbs = {0};
static ScreenMode sSettingsBack = SCREEN_TITLE;
static float sTitleTime = 0.0f;
static int sDeathWave = 1;
static int sDeathKills = 0;
static bool sDeathBest = false;
static bool sShouldQuit = false;

static Vector3 sSwoopPos0 = { 2.0f, 0.0f, 24.0f };
static Vector3 sSwoopLook0 = { -1.5f, 2.2f, 8.0f };

void title_init(TitleCallbacks cbs) {
    sCbs = cbs;
    gScreen = SCREEN_TITLE;
    gSwoopT = -1.0f;
    sTitleTime = 0.0f;
}

void title_set_screen(ScreenMode mode) {
    if (mode == SCREEN_SETTINGS && gScreen != SCREEN_SETTINGS) {
        sSettingsBack = gScreen;
    }
    gScreen = mode;
}

void title_start_swoop(void) {
    sSwoopPos0 = gScene.camera.position;
    sSwoopLook0 = gScene.camera.target;
    gSwoopT = 0.0f;
}

void title_set_death_info(int wave, int kills, bool isNewBest) {
    sDeathWave = wave;
    sDeathKills = kills;
    sDeathBest = isNewBest;
}

static bool ui_button(Rectangle r, const char *label, bool primary) {
    Vector2 mp = GetMousePosition();
    bool hover = CheckCollisionPointRec(mp, r);
    if (hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        sfx_ui("click");
        return true;
    }

    Color bg = primary
        ? (hover ? hex_color(0xd6453c, 240) : hex_color(0xa8322a, 225))
        : (hover ? (Color){ 46, 52, 64, 235 } : (Color){ 24, 28, 36, 220 });
    Color border = hover ? hex_color(0xe8d28a, 255) : (Color){ 90, 98, 114, 200 };

    DrawRectangleRounded(r, 0.14f, 6, bg);
    DrawRectangleRoundedLinesEx(r, 0.14f, 6, hover ? 2.0f : 1.0f, border);

    float tw = font_measure(gFontAnton, label, 24.0f, 2.0f);
    font_draw(gFontAnton, label, r.x + (r.width - tw) * 0.5f, r.y + (r.height - 24.0f) * 0.5f,
              24.0f, 2.0f, RAYWHITE);
    return false;
}

void title_update(float dt, Camera3D *cam, Vector3 playerPos) {
    sTitleTime += dt;
    float t = sTitleTime;

    if (gScreen == SCREEN_TITLE || gScreen == SCREEN_HOWTO ||
        (gScreen == SCREEN_SETTINGS && sSettingsBack == SCREEN_TITLE)) {
        cam->position = (Vector3){
            2.0f + 0.9f * sinf(t * 0.045f),
            2.0f + 0.2f * sinf(t * 0.07f) + 0.03f * sinf(t * 1.3f),
            24.0f - 1.2f * sinf(t * 0.031f)
        };
        cam->target = (Vector3){
            -1.5f + 0.7f * sinf(t * 0.045f + 0.6f),
            2.2f + 0.02f * sinf(t * 1.1f),
            8.0f
        };
        cam->fovy = TITLE_FOV;
        return;
    }

    if (gSwoopT >= 0.0f) {
        gSwoopT += dt;
        float u = clampf(gSwoopT / SWOOP_TIME, 0.0f, 1.0f);
        float e = ease_smooth(u);
        Vector3 destPos = {
            playerPos.x + CAM_OFFSET_X,
            playerPos.y + CAM_OFFSET_Y,
            playerPos.z + CAM_OFFSET_Z
        };
        cam->position = Vector3Lerp(sSwoopPos0, destPos, e);
        cam->target   = Vector3Lerp(sSwoopLook0, playerPos, e);
        cam->fovy     = lerpf(TITLE_FOV, GAME_FOV, e);
        if (u >= 1.0f) gSwoopT = -1.0f;
    }
}

void title_draw(void) {
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();

    if (gScreen == SCREEN_PLAYING) return;

    if (gScreen == SCREEN_TITLE) {
        DrawRectangle(0, 0, sw, sh, (Color){ 4, 6, 10, 120 });

        const char *titleTop = "ABANDON";
        const char *titleMain = "ALL HOPE";
        float ttw = font_measure(gFontAnton, titleTop, 57.0f, 15.0f);
        float tmw = font_measure(gFontAnton, titleMain, 144.0f, 6.0f);
        font_draw_distressed(gFontAnton, titleTop, ((float)sw - ttw) * 0.5f, (float)sh * 0.5f - 302.0f,
                             57.0f, 15.0f, hex_color(0xe9e2d6, 255), 11u);
        font_draw_distressed(gFontAnton, titleMain, ((float)sw - tmw) * 0.5f, (float)sh * 0.5f - 245.0f,
                             144.0f, 6.0f, hex_color(0xd6453c, 255), 29u);

        const char *sub = "HOLD THE CABIN. SURVIVE THE NIGHT.";
        float suw = font_measure(gFontJersey, sub, 20.0f, 2.5f);
        font_draw(gFontJersey, sub, ((float)sw - suw) * 0.5f, (float)sh * 0.5f - 102.0f,
                  20.0f, 2.5f, hex_color(0xe8d28a, 230));

        if (gSettings.bestKills > 0) {
            const char *best = TextFormat("BEST NIGHT: WAVE %d  ·  %d KILLS", gSettings.bestWave, gSettings.bestKills);
            float bwBest = font_measure(gFontJersey, best, 18.0f, 2.0f);
            font_draw(gFontJersey, best, ((float)sw - bwBest) * 0.5f, (float)sh * 0.5f - 72.0f,
                      18.0f, 2.0f, (Color){ 175, 188, 205, 220 });
        }

        float bw = 260.0f, bh = 48.0f, bx = ((float)sw - bw) * 0.5f;
        float by = (float)sh * 0.5f - 28.0f;

        if (ui_button((Rectangle){ bx, by, bw, bh }, "PLAY", true) ||
            IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
            if (sCbs.onPlay) sCbs.onPlay();
        }
        if (ui_button((Rectangle){ bx, by + 64.0f, bw, bh }, "HOW TO PLAY", false)) {
            title_set_screen(SCREEN_HOWTO);
        }
        if (ui_button((Rectangle){ bx, by + 128.0f, bw, bh }, "SETTINGS", false)) {
            title_set_screen(SCREEN_SETTINGS);
        }
        if (ui_button((Rectangle){ bx, by + 192.0f, bw, bh }, "QUIT", false)) {
            sShouldQuit = true;
        }
        return;
    }

    if (gScreen == SCREEN_HOWTO) {
        DrawRectangle(0, 0, sw, sh, (Color){ 5, 7, 11, 205 });
        int pw = 540, ph = 370;
        int px = (sw - pw) / 2, py = (sh - ph) / 2;
        DrawRectangleRounded((Rectangle){ (float)px, (float)py, (float)pw, (float)ph }, 0.04f, 8, (Color){ 18, 21, 28, 242 });
        DrawRectangleRoundedLinesEx((Rectangle){ (float)px, (float)py, (float)pw, (float)ph }, 0.04f, 8, 1.5f, (Color){ 90, 98, 114, 210 });

        const char *hdr = "HOW TO PLAY";
        DrawText(hdr, (sw - MeasureText(hdr, 28)) / 2, py + 24, 28, hex_color(0xe8d28a, 255));

        const char *lines[] = {
            "WASD / ARROWS       Move survivor",
            "MOUSE               Aim rifle",
            "LEFT CLICK          Fire (hold for auto)",
            "R                   Reload magazine",
            "M                   Mute / unmute audio",
            "ESC / P             Pause game",
            "",
            "Zombies break down doors and climb through windows.",
            "Collect drops and choose an upgrade after each wave."
        };
        for (int i = 0; i < 9; i++) {
            Color c = (i < 6) ? (Color){ 215, 222, 232, 255 } : hex_color(0x8fae6b, 255);
            DrawText(lines[i], px + 36, py + 76 + i * 26, 17, c);
        }

        if (ui_button((Rectangle){ (float)(sw - 180) * 0.5f, (float)(py + ph - 64), 180.0f, 42.0f }, "BACK", false)) {
            title_set_screen(SCREEN_TITLE);
        }
        return;
    }

    if (gScreen == SCREEN_SETTINGS) {
        DrawRectangle(0, 0, sw, sh, (Color){ 5, 7, 11, 205 });
        int pw = 460, ph = 330;
        int px = (sw - pw) / 2, py = (sh - ph) / 2;
        DrawRectangleRounded((Rectangle){ (float)px, (float)py, (float)pw, (float)ph }, 0.04f, 8, (Color){ 18, 21, 28, 242 });
        DrawRectangleRoundedLinesEx((Rectangle){ (float)px, (float)py, (float)pw, (float)ph }, 0.04f, 8, 1.5f, (Color){ 90, 98, 114, 210 });

        const char *hdr = "SETTINGS";
        DrawText(hdr, (sw - MeasureText(hdr, 28)) / 2, py + 24, 28, hex_color(0xe8d28a, 255));

        // Master Volume
        int rowY = py + 86;
        DrawText("MASTER VOLUME", px + 36, rowY + 8, 18, RAYWHITE);
        if (ui_button((Rectangle){ (float)(px + pw - 165), (float)rowY, 38.0f, 36.0f }, "-", false)) {
            if (gSettings.masterVol > 0) gSettings.masterVol--;
            storage_save();
            if (sCbs.onSettingsChanged) sCbs.onSettingsChanged();
        }
        DrawText(TextFormat("%d", gSettings.masterVol), px + pw - 110, rowY + 8, 20, hex_color(0xe8d28a, 255));
        if (ui_button((Rectangle){ (float)(px + pw - 74), (float)rowY, 38.0f, 36.0f }, "+", false)) {
            if (gSettings.masterVol < 10) gSettings.masterVol++;
            storage_save();
            if (sCbs.onSettingsChanged) sCbs.onSettingsChanged();
        }

        // Music Volume
        rowY += 56;
        DrawText("MUSIC VOLUME", px + 36, rowY + 8, 18, RAYWHITE);
        if (ui_button((Rectangle){ (float)(px + pw - 165), (float)rowY, 38.0f, 36.0f }, "-", false)) {
            if (gSettings.musicVol > 0) gSettings.musicVol--;
            storage_save();
            if (sCbs.onSettingsChanged) sCbs.onSettingsChanged();
        }
        DrawText(TextFormat("%d", gSettings.musicVol), px + pw - 110, rowY + 8, 20, hex_color(0xe8d28a, 255));
        if (ui_button((Rectangle){ (float)(px + pw - 74), (float)rowY, 38.0f, 36.0f }, "+", false)) {
            if (gSettings.musicVol < 10) gSettings.musicVol++;
            storage_save();
            if (sCbs.onSettingsChanged) sCbs.onSettingsChanged();
        }

        // Screen Shake
        rowY += 56;
        DrawText("SCREEN SHAKE", px + 36, rowY + 8, 18, RAYWHITE);
        if (ui_button((Rectangle){ (float)(px + pw - 138), (float)rowY, 102.0f, 36.0f }, gSettings.shake ? "ON" : "OFF", false)) {
            gSettings.shake = !gSettings.shake;
            storage_save();
            if (sCbs.onSettingsChanged) sCbs.onSettingsChanged();
        }

        if (ui_button((Rectangle){ (float)(sw - 180) * 0.5f, (float)(py + ph - 62), 180.0f, 42.0f }, "BACK", false)) {
            gScreen = sSettingsBack;
        }
        return;
    }

    if (gScreen == SCREEN_PAUSED) {
        DrawRectangle(0, 0, sw, sh, (Color){ 6, 8, 12, 175 });
        const char *hdr = "PAUSED";
        float hw = font_measure(gFontAnton, hdr, 117.0f, 12.0f);
        font_draw_distressed(gFontAnton, hdr, ((float)sw - hw) * 0.5f, (float)sh * 0.5f - 205.0f,
                             117.0f, 12.0f, hex_color(0xe9e2d6, 255), 53u);

        float bw = 240.0f, bh = 46.0f, bx = ((float)sw - bw) * 0.5f;
        float by = (float)sh * 0.5f - 62.0f;
        if (ui_button((Rectangle){ bx, by, bw, bh }, "RESUME", true)) {
            if (sCbs.onResume) sCbs.onResume();
        }
        if (ui_button((Rectangle){ bx, by + 62.0f, bw, bh }, "SETTINGS", false)) {
            title_set_screen(SCREEN_SETTINGS);
        }
        if (ui_button((Rectangle){ bx, by + 124.0f, bw, bh }, "QUIT TO TITLE", false)) {
            if (sCbs.onQuitToTitle) sCbs.onQuitToTitle();
        }
        return;
    }

    if (gScreen == SCREEN_DEAD) {
        DrawRectangle(0, 0, sw, sh, (Color){ 18, 4, 6, 185 });
        const char *hdr = "YOU DIED";
        float hw = font_measure(gFontAnton, hdr, 132.0f, 12.0f);
        font_draw_distressed(gFontAnton, hdr, ((float)sw - hw) * 0.5f, (float)sh * 0.5f - 232.0f,
                             132.0f, 12.0f, hex_color(0xb8332b, 255), 71u);

        const char *stats = TextFormat("YOU REACHED WAVE %d  ·  %d KILLS", sDeathWave, sDeathKills);
        float stw = font_measure(gFontJersey, stats, 24.0f, 3.0f);
        font_draw(gFontJersey, stats, ((float)sw - stw) * 0.5f, (float)sh * 0.5f - 94.0f,
                  24.0f, 3.0f, hex_color(0xd6453c, 255));

        const char *bestTxt = sDeathBest
            ? "NEW BEST NIGHT!"
            : TextFormat("BEST: WAVE %d  ·  %d KILLS", gSettings.bestWave, gSettings.bestKills);
        float btw = font_measure(gFontJersey, bestTxt, 19.0f, 3.0f);
        font_draw(gFontJersey, bestTxt, ((float)sw - btw) * 0.5f, (float)sh * 0.5f - 58.0f,
                  19.0f, 3.0f, sDeathBest ? hex_color(0xe8d28a, 255) : (Color){ 175, 185, 200, 220 });

        float bw = 240.0f, bh = 46.0f, bx = ((float)sw - bw) * 0.5f;
        float by = (float)sh * 0.5f + 4.0f;
        if (ui_button((Rectangle){ bx, by, bw, bh }, "TRY AGAIN", true)) {
            if (sCbs.onRestart) sCbs.onRestart();
        }
        if (ui_button((Rectangle){ bx, by + 62.0f, bw, bh }, "QUIT TO TITLE", false)) {
            if (sCbs.onQuitToTitle) sCbs.onQuitToTitle();
        }
    }
}

bool title_should_quit(void) {
    return sShouldQuit;
}
