#include "game.h"
#include "core/math_util.h"
#include "core/scene.h"
#include "core/state.h"
#include "core/storage.h"
#include "world/world.h"
#include "world/solids.h"
#include "world/openings.h"
#include "world/weather.h"
#include "effects/particles.h"
#include "entities/player.h"
#include "entities/weapon.h"
#include "entities/zombies.h"
#include "entities/drops.h"
#include "audio/synth.h"
#include "audio/sfx.h"
#include "audio/music.h"
#include "ui/hud.h"
#include "ui/upgrades.h"
#include "ui/title.h"
#include "raymath.h"
#include <math.h>
#include <stddef.h>

#define MAX_ALIVE    30
#define HURT_IFRAMES 1.5f

static Vector3 sAimPoint = { 0.0f, 0.9f, 5.0f };
static bool sWaveHealed = false;

static const char *wave_omen(int n) {
    if (n == 1)  return "they are already here";
    if (n == 5)  return "it starts to rain";
    if (n == 8)  return "the rain sets in";
    if (n == 12) return "the storm breaks";
    if (n == 15) return "god help you";
    return "it gets worse";
}

static void start_wave(int n) {
    gState.wave = n;
    gState.zombieSpeed = fminf(5.2f, 2.3f + (float)n * 0.18f);
    gState.zombieHp = 2.0f + (float)(n - 1) * 0.4f;
    int need = 5 + n * 3 - gZombieCount;
    gState.toSpawn = need > 0 ? need : 0;
    gState.spawnTimer = 0.0f;
    gState.intermission = 0.0f;

    weather_set(storm_for_wave(n));
    sfx_wave_start(n);
    hud_wave_banner(TextFormat("WAVE %d", n), wave_omen(n));
}

static void reset_game(bool forTitle) {
    state_reset_stats();
    zombies_clear();
    drops_clear();
    weapon_clear();
    effects_clear();
    reset_openings();
    hud_reset();

    gPlayer.position = (Vector3){ 0.0f, 0.0f, 0.0f };
    gPlayer.rotY = 0.0f;
    gPlayer.visible = true;
    sAimPoint = (Vector3){ 0.0f, 0.9f, 5.0f };

    if (forTitle) {
        gState.running = false;
        gState.paused = false;
        weather_set(0.0f);
        music_set_mode(MUSIC_TITLE);
        for (int i = 0; i < TITLE_ZOMBIES; i++) {
            Vector3 p = {
                rndf(-16.0f, 16.0f),
                0.0f,
                rndf(8.0f, 18.0f)
            };
            zombies_spawn(&p);
        }
    } else {
        gState.running = true;
        gState.paused = false;
        music_set_mode(MUSIC_COMBAT);
        start_wave(1);
    }
}

static void on_drop_collect(DropKind kind, const char *name, Color color) {
    (void)color;
    sfx_pickup((int)kind);
    hud_toast(name);
}

static bool hurt_player(void) {
    if (!gState.running || gState.dmgCd > 0.0f) return false;
    gState.hp--;
    gState.dmgCd = HURT_IFRAMES;
    gState.shake = 0.5f;
    hud_damage_flash();
    sfx_hurt();

    if (gState.hp <= 0) {
        gState.running = false;
        gPlayer.visible = true;
        sfx_death();
        music_set_mode(MUSIC_DEAD);
        bool isBest = storage_record_run(gState.wave, gState.kills);
        title_set_death_info(gState.wave, gState.kills, isBest);
        title_set_screen(SCREEN_DEAD);
    }
    return true;
}

static void apply_audio_settings(void) {
    synth_set_volumes(gSettings.masterVol, gSettings.musicVol, gSettings.muted);
}

static void cb_play(void) {
    reset_game(false);
    title_set_screen(SCREEN_PLAYING);
    title_start_swoop();
}

static void cb_resume(void) {
    gState.paused = false;
    synth_set_paused(false);
    title_set_screen(SCREEN_PLAYING);
}

static void cb_restart(void) {
    reset_game(false);
    title_set_screen(SCREEN_PLAYING);
}

static void cb_quit_to_title(void) {
    gState.paused = false;
    synth_set_paused(false);
    upgrades_close();
    reset_game(true);
    title_set_screen(SCREEN_TITLE);
}

void game_init(void) {
    storage_load();
    state_reset_stats();
    scene_init();
    world_init();
    weather_init();
    effects_init();
    player_init();
    zombies_init();
    drops_init();
    weapon_clear();
    hud_init();

    synth_init();
    music_init();
    apply_audio_settings();

    drops_set_collect_callback(on_drop_collect);
    zombies_set_hurt_handler(hurt_player);

    title_init((TitleCallbacks){
        .onPlay = cb_play,
        .onResume = cb_resume,
        .onRestart = cb_restart,
        .onQuitToTitle = cb_quit_to_title,
        .onSettingsChanged = apply_audio_settings,
    });

    reset_game(true);
    title_set_screen(SCREEN_TITLE);
}

void game_update(float dt) {
    // Global mute toggle
    if (IsKeyPressed(KEY_M)) {
        gSettings.muted = !gSettings.muted;
        storage_save();
        apply_audio_settings();
        if (gScreen == SCREEN_PLAYING) {
            hud_toast(gSettings.muted ? "AUDIO MUTED" : "AUDIO UNMUTED");
        }
    }

    // Pause / Resume toggle
    if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_P)) {
        if (gScreen == SCREEN_PLAYING && gState.running && !gState.choosing) {
            gState.paused = true;
            synth_set_paused(true);
            title_set_screen(SCREEN_PAUSED);
        } else if (gScreen == SCREEN_PAUSED) {
            cb_resume();
        }
    }

    bool titling = (gScreen == SCREEN_TITLE || gScreen == SCREEN_HOWTO || gScreen == SCREEN_SETTINGS);
    bool paused = (gScreen == SCREEN_PAUSED || (gScreen == SCREEN_SETTINGS && gState.paused));

    if (paused) {
        return;
    }

    if (gState.choosing) {
        if (upgrades_update(dt)) {
            music_set_mode(MUSIC_COMBAT);
            start_wave(gState.wave + 1);
        }
        weather_update(dt, false);
        effects_update(dt);
        world_update(dt, gWeather.level);
        update_openings(dt);
        hud_update(dt);
        return;
    }

    // Gameplay input & movement
    float px = gPlayer.position.x, pz = gPlayer.position.z;
    if (gState.running && gScreen == SCREEN_PLAYING) {
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
        push_out_of_solids(&gPlayer.position);
        float lim = WORLD_HALF - 0.7f;
        gPlayer.position.x = clampf(gPlayer.position.x, -lim, lim);
        gPlayer.position.z = clampf(gPlayer.position.z, -lim, lim);

        Ray ray = GetScreenToWorldRay(GetMousePosition(), gScene.camera);
        if (fabsf(ray.direction.y) > 1e-5f) {
            float t = (0.9f - ray.position.y) / ray.direction.y;
            if (t > 0.0f) {
                sAimPoint.x = ray.position.x + ray.direction.x * t;
                sAimPoint.z = ray.position.z + ray.direction.z * t;
                gPlayer.rotY = atan2f(sAimPoint.x - gPlayer.position.x, sAimPoint.z - gPlayer.position.z);
            }
        }

        if (IsKeyPressed(KEY_R)) weapon_start_reload();
        gState.fireCd -= dt;
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) weapon_fire(sAimPoint);
        if (gState.reloading) {
            gState.reloadT += dt;
            if (gState.reloadT >= gState.reloadTime) weapon_finish_reload();
        }
    }

    // Damage i-frames & flicker
    if (gState.dmgCd > 0.0f) {
        gState.dmgCd = fmaxf(0.0f, gState.dmgCd - dt);
        gPlayer.visible = (gState.dmgCd <= 0.0f) || ((int)(gState.dmgCd * 16.0f) % 2 == 0);
    } else {
        gPlayer.visible = true;
    }

    gState.pDamage = fmaxf(0.0f, gState.pDamage - dt);
    gState.pAmmo = fmaxf(0.0f, gState.pAmmo - dt);

    // Wave spawning & intermission
    if (gState.running && gScreen == SCREEN_PLAYING) {
        if (gState.toSpawn > 0) {
            gState.spawnTimer -= dt;
            if (gState.spawnTimer <= 0.0f && gZombieCount < MAX_ALIVE) {
                zombies_spawn(NULL);
                gState.toSpawn--;
                gState.spawnTimer = fmaxf(0.35f, 1.25f - (float)gState.wave * 0.06f);
            }
        } else if (gZombieCount == 0 && !gState.choosing) {
            if (gState.intermission <= 0.0f) {
                gState.intermission = 1.6f;
                sWaveHealed = gState.hp < gState.maxHp;
                if (sWaveHealed) gState.hp++;
                sfx_wave_clear();
                music_set_mode(MUSIC_CALM);
                hud_wave_banner("WAVE CLEARED", sWaveHealed ? "+1 health · catch your breath" : "catch your breath");
            } else {
                gState.intermission -= dt;
                if (gState.intermission <= 0.0f) {
                    upgrades_open(sWaveHealed);
                    if (!gState.choosing) {
                        music_set_mode(MUSIC_COMBAT);
                        start_wave(gState.wave + 1);
                    }
                }
            }
        }
    }

    weapon_update_bullets(dt);
    zombies_update(dt, gState.running, titling);
    drops_update(dt);
    effects_update(dt);
    weather_update(dt, gState.running);

    float inv = dt > 0.0f ? 1.0f / dt : 0.0f;
    player_animate(dt, (gPlayer.position.x - px) * inv, (gPlayer.position.z - pz) * inv);

    world_update(dt, gWeather.level);
    update_openings(dt);
    hud_update(dt);
    music_update_state(gState.wave, gZombieCount, gState.hp <= 1, dt);

    // Camera update (title drift, swoop, or follow + shake)
    title_update(dt, &gScene.camera, gPlayer.position);
    if (!titling && gSwoopT < 0.0f) {
        float sx = 0.0f, sz = 0.0f;
        if (gState.shake > 0.0f) {
            gState.shake = fmaxf(0.0f, gState.shake - dt * 2.5f);
            if (gSettings.shake) {
                float mag = gState.shake * 0.45f;
                sx = rndf(-mag, mag);
                sz = rndf(-mag, mag);
            }
        }
        gScene.camera.target = (Vector3){ gPlayer.position.x + sx, gPlayer.position.y, gPlayer.position.z + sz };
        gScene.camera.position = (Vector3){
            gPlayer.position.x + CAM_OFFSET_X + sx,
            gPlayer.position.y + CAM_OFFSET_Y,
            gPlayer.position.z + CAM_OFFSET_Z + sz
        };
        gScene.camera.fovy = GAME_FOV;
    }

    scene_update_lighting(gWorld.lanternPos, gWorld.lanternIntensity,
                          gPlayer.muzzleLightPos, gPlayer.muzzleLightInt, gPlayer.flashColor);
}

void game_draw(void) {
    BeginDrawing();
    ClearBackground(gScene.fogColor);

    BeginMode3D(gScene.camera);
    world_draw(gPlayer.position);
    player_draw();
    zombies_draw();
    drops_draw();
    weapon_draw_bullets();
    effects_draw();
    weather_draw();
    EndMode3D();

    if (gScreen == SCREEN_PLAYING || gScreen == SCREEN_PAUSED) {
        hud_draw();
    }
    if (gState.choosing) {
        upgrades_draw();
    }
    title_draw();

    EndDrawing();
}

void game_cleanup(void) {
    synth_cleanup();
    drops_cleanup();
    zombies_cleanup();
    player_cleanup();
    effects_cleanup();
    world_cleanup();
    scene_cleanup();
}
