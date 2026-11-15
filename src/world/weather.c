#include "weather.h"
#include "core/math_util.h"
#include "core/scene.h"
#include "entities/player.h"
#include "audio/synth.h"
#include "audio/sfx.h"
#include "rlgl.h"
#include <math.h>
#include <string.h>

#define RAIN_COUNT 2400
#define RAIN_AREA 72.0f
#define RAIN_TOP 16.0f
#define SPLASH_COUNT 240

typedef struct {
    float sx, sy, sz;
} RainDrop;

typedef struct {
    float x, z, born;
} SplashRing;

WeatherState gWeather = {0};
static RainDrop gRain[RAIN_COUNT];
static SplashRing gSplashes[SPLASH_COUNT];

static const float STORM_STEPS[5][2] = {
    { 4.0f, 0.0f },
    { 5.0f, 0.15f },
    { 8.0f, 0.4f },
    { 12.0f, 0.7f },
    { 15.0f, 1.0f }
};

void weather_init(void) {
    weather_set(0.0f);
    for (int i = 0; i < RAIN_COUNT; i++) {
        gRain[i] = (RainDrop){ rnd01(), rnd01(), rnd01() };
    }
    for (int i = 0; i < SPLASH_COUNT; i++) {
        gSplashes[i] = (SplashRing){ 0.0f, 0.0f, -9.0f };
    }
}

void weather_set(float level) {
    gWeather.level = level;
    gWeather.target = level;
    gWeather.flashT = -1.0f;
    gWeather.flash = 0.0f;
    gWeather.thunderIn = -1.0f;
    gWeather.nextBolt = 8.0f;
}

float storm_for_wave(int wave) {
    float w = (float)wave;
    for (int i = 1; i < 5; i++) {
        float w0 = STORM_STEPS[i - 1][0], s0 = STORM_STEPS[i - 1][1];
        float w1 = STORM_STEPS[i][0],     s1 = STORM_STEPS[i][1];
        if (w <= w1) {
            return w <= w0 ? s0 : s0 + (s1 - s0) * (w - w0) / (w1 - w0);
        }
    }
    return 1.0f;
}

void weather_strike_lightning(void) {
    gWeather.flashT = 0.0f;
    gWeather.thunderNear = rnd01();
    gWeather.thunderIn = 0.15f + (1.0f - gWeather.thunderNear) * 2.2f;
}

void weather_update(float dt, bool running) {
    WeatherState *w = &gWeather;
    w->level += clampf(w->target - w->level, -dt * 0.08f, dt * 0.08f);
    float L = w->level;
    w->t += dt;

    w->gust = 1.0f + L * (0.45f * sinf(w->t * 0.63f) + 0.3f * sinf(w->t * 1.71f + 2.0f) + 0.15f * sinf(w->t * 4.1f));

    float speed = 11.0f + 13.0f * L;
    w->drop += speed * dt;

    w->splashDue += L * 520.0f * dt;
    while (w->splashDue >= 1.0f) {
        w->splashDue -= 1.0f;
        w->splashI = (w->splashI + 1) % SPLASH_COUNT;
        gSplashes[w->splashI] = (SplashRing){
            gPlayer.position.x + rndf(-26.0f, 26.0f),
            gPlayer.position.z + rndf(-22.0f, 14.0f),
            w->t
        };
    }

    if (L >= BOLT_LEVEL - 0.001f && running) {
        w->nextBolt -= dt;
        if (w->nextBolt <= 0.0f) {
            weather_strike_lightning();
            float k = (L - BOLT_LEVEL) / (1.0f - BOLT_LEVEL);
            w->nextBolt = lerpf(16.0f, 3.5f, k) * rndf(0.6f, 1.4f);
        }
    }

    if (w->thunderIn >= 0.0f) {
        w->thunderIn -= dt;
        if (w->thunderIn < 0.0f) sfx_thunder(w->thunderNear);
    }

    w->soundT -= dt;
    if (w->soundT <= 0.0f) {
        w->soundT = 0.2f;
        synth_set_weather(L, w->gust);
    }

    if (w->flashT >= 0.0f) {
        w->flashT += dt;
        float t = w->flashT;
        w->flash = expf(-t * 14.0f) + (t > 0.13f ? 0.8f * expf(-(t - 0.13f) * 9.0f) : 0.0f);
        if (t > 1.2f) {
            w->flashT = -1.0f;
            w->flash = 0.0f;
        }
    }

    gScene.fogNear = 32.0f - 14.0f * L;
    gScene.fogFar  = 62.0f - 18.0f * L;
    float fMix = clampf(w->flash * 0.7f, 0.0f, 1.0f);
    gScene.fogColor = (Color){
        (uint8_t)lerpf(5.0f, 58.0f, fMix),
        (uint8_t)lerpf(6.0f, 68.0f, fMix),
        (uint8_t)lerpf(10.0f, 88.0f, fMix),
        255
    };
    gScene.moonIntensity = 1.0f - 0.39f * L + w->flash * 5.0f;
    gScene.ambientIntensity = 1.0f - 0.23f * L + w->flash * 1.0f;
}

void weather_draw(void) {
    float L = gWeather.level;
    if (L <= 0.005f) return;

    float speed = 11.0f + 13.0f * L;
    float len = 0.25f + 1.0f * L;
    float opacity = 0.3f + 0.35f * L;
    float wx = 7.0f * L * L * gWeather.gust;
    float wz = 2.5f * L * L * gWeather.gust;

    Vector3 trail = Vector3Scale(Vector3Normalize((Vector3){ -wx, speed, -wz }), len);
    Vector3 center = { gPlayer.position.x, 0.0f, gPlayer.position.z - 4.0f };

    int drawCount = (int)roundf((float)RAIN_COUNT * L);
    if (drawCount > RAIN_COUNT) drawCount = RAIN_COUNT;

    rlDrawRenderBatchActive();
    rlBegin(RL_LINES);
    rlColor4ub(204, 222, 242, (uint8_t)(opacity * 255.0f));
    for (int i = 0; i < drawCount; i++) {
        RainDrop r = gRain[i];
        float y = fmodf(r.sz * RAIN_TOP - gWeather.drop * (0.85f + 0.3f * r.sx), RAIN_TOP);
        if (y < 0.0f) y += RAIN_TOP;
        float px = center.x + (r.sx - 0.5f) * RAIN_AREA + wx * (RAIN_TOP - y) / speed;
        float pz = center.z + (r.sy - 0.5f) * RAIN_AREA + wz * (RAIN_TOP - y) / speed;
        rlVertex3f(px, y, pz);
        rlVertex3f(px + trail.x, y + trail.y, pz + trail.z);
    }
    rlEnd();

    // Ground splashes
    BeginBlendMode(BLEND_ADDITIVE);
    for (int i = 0; i < SPLASH_COUNT; i++) {
        SplashRing *s = &gSplashes[i];
        float k = (gWeather.t - s->born) / 0.22f;
        if (k < 0.0f || k >= 1.0f) continue;
        float rad = 0.035f * (1.0f + 2.5f * k);
        uint8_t a = (uint8_t)((1.0f - k) * (1.0f - k) * (0.35f + 0.5f * L) * 180.0f);
        DrawCircle3D((Vector3){ s->x, 0.03f, s->z }, rad, (Vector3){ 1, 0, 0 }, 90.0f,
                     (Color){ 191, 208, 224, a });
    }
    EndBlendMode();
}
