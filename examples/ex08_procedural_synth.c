// Lesson 8: Procedural AudioStream synth (band-limited pulse/tri, LFSR noise, SVF, rapid fire & double-damage)
#include "raylib.h"
#include <math.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#define SR 44100
#define MAX_V 32
#define ECHO_LEN 16537 /* 0.375s at 44100 Hz */

typedef enum { W_SINE = 0, W_TRI, W_PULSE25, W_NOISE_LONG, W_NOISE_METAL } WaveKind;
typedef enum { F_NONE = 0, F_LOWPASS, F_BANDPASS } FiltKind;

typedef struct {
    bool active;
    WaveKind kind;
    FiltKind filt;
    float f, f2, phase, t, dur, delay, vol, wet;
    float lp, bp, filtF, filtF2, filtQ;
    int noisePos;
} Voice;

static Voice gVoices[MAX_V];
static float gNoiseLong[SR];
static float gNoiseMetal[93 * 64];
static float gEcho[ECHO_LEN];
static int gEchoIdx = 0;
static pthread_mutex_t gMutex;

static void spawn_voice(Voice v) {
    pthread_mutex_lock(&gMutex);
    for (int i = 0; i < MAX_V; i++) {
        if (!gVoices[i].active) {
            v.active = true;
            if (v.filtQ <= 0.0f) v.filtQ = 0.7f;
            v.noisePos = GetRandomValue(0, 4096);
            gVoices[i] = v;
            break;
        }
    }
    pthread_mutex_unlock(&gMutex);
}

static void play_shot(bool hot, bool spread) {
    float v = 0.65f;
    // Hot (Double Damage) starts the pulse sweep higher (850 Hz vs 680 Hz)
    spawn_voice((Voice){ .kind = W_PULSE25, .f = hot ? 850.0f : 680.0f, .f2 = 45.0f, .dur = 0.11f, .vol = 0.85f * v, .wet = 0.25f });
    spawn_voice((Voice){ .kind = W_NOISE_METAL, .filt = F_BANDPASS, .filtF = 3800.0f, .filtQ = 1.0f, .dur = 0.06f, .vol = 0.9f * v, .wet = 0.25f });
    spawn_voice((Voice){ .kind = W_NOISE_LONG, .filt = F_LOWPASS, .filtF = 2400.0f, .filtF2 = 300.0f, .dur = 0.18f, .vol = 0.8f * v, .wet = 0.25f });
    spawn_voice((Voice){ .kind = W_SINE, .f = 120.0f, .f2 = 38.0f, .dur = 0.14f, .vol = 0.9f * v, .wet = 0.20f });
    if (spread) {
        spawn_voice((Voice){ .kind = W_NOISE_LONG, .delay = 0.01f, .filt = F_LOWPASS, .filtF = 1800.0f, .dur = 0.22f, .vol = 0.5f * v, .wet = 0.25f });
    }
}

static void audio_cb(void *buf, unsigned int frames) {
    short *out = (short *)buf;
    const float dt = 1.0f / (float)SR;
    pthread_mutex_lock(&gMutex);
    for (unsigned int s = 0; s < frames; s++) {
        float dry = 0.0f, wetSend = 0.0f;
        for (int i = 0; i < MAX_V; i++) {
            Voice *v = &gVoices[i];
            if (!v->active) continue;
            if (v->delay > 0.0f) { v->delay -= dt; continue; }
            float u = v->t / v->dur;
            if (u >= 1.0f) { v->active = false; continue; }

            float freq = v->f2 > 0.0f ? v->f * powf(v->f2 / v->f, u) : v->f;
            v->phase = fmodf(v->phase + freq * dt, 1.0f);

            float raw = 0.0f;
            if (v->kind == W_SINE) raw = sinf(2.0f * PI * v->phase);
            else if (v->kind == W_TRI) raw = 1.0f - 4.0f * fabsf(v->phase - 0.5f);
            else if (v->kind == W_PULSE25) raw = (v->phase < 0.25f) ? 1.0f : -1.0f;
            else if (v->kind == W_NOISE_LONG) raw = gNoiseLong[(v->noisePos + (int)(v->t * (float)SR)) % SR];
            else raw = gNoiseMetal[(v->noisePos + (int)(v->t * (float)SR)) % (93 * 64)];

            if (v->filt != F_NONE) {
                float ff = v->filtF2 > 0.0f ? v->filtF * powf(v->filtF2 / v->filtF, u) : v->filtF;
                float fc = fminf(0.45f, 2.0f * sinf(PI * ff * dt));
                v->lp += fc * v->bp;
                float hp = raw - v->lp - (1.0f / v->filtQ) * v->bp;
                v->bp += fc * hp;
                raw = (v->filt == F_LOWPASS) ? v->lp : v->bp;
            }
            float amp = expf(-u * 5.0f) * v->vol;
            dry += raw * amp;
            wetSend += raw * amp * v->wet;
            v->t += dt;
        }

        // Dotted-8th echo tail
        float echoOut = gEcho[gEchoIdx];
        gEcho[gEchoIdx] = wetSend + echoOut * 0.28f;
        gEchoIdx = (gEchoIdx + 1) % ECHO_LEN;

        float lim = tanhf(dry + echoOut * 0.45f);
        short sample = (short)(lim * 24000.0f);
        out[s * 2 + 0] = sample;
        out[s * 2 + 1] = sample;
    }
    pthread_mutex_unlock(&gMutex);
}

int main(void) {
    InitWindow(960, 540, "Ex 08: Procedural AudioStream Synth");
    SetTargetFPS(60);

    // 15-bit LFSR noise (long + 93-step metallic)
    uint16_t lfsr = 1;
    for (int i = 0; i < SR; i++) {
        uint16_t bit = (uint16_t)((lfsr ^ (lfsr >> 1)) & 1u);
        lfsr = (uint16_t)((lfsr >> 1) | (bit << 14));
        gNoiseLong[i] = (lfsr & 1u) ? 1.0f : -1.0f;
    }
    lfsr = 1;
    for (int i = 0; i < 93 * 64; i++) {
        uint16_t bit = (uint16_t)((lfsr ^ (lfsr >> 6)) & 1u);
        lfsr = (uint16_t)((lfsr >> 1) | (bit << 14));
        gNoiseMetal[i] = (lfsr & 1u) ? 1.0f : -1.0f;
    }

    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&gMutex, &attr);
    pthread_mutexattr_destroy(&attr);

    InitAudioDevice();
    SetAudioStreamBufferSizeDefault(1024);
    AudioStream stream = LoadAudioStream(SR, 16, 2);
    SetAudioStreamCallback(stream, audio_cb);
    PlayAudioStream(stream);

    bool doubleDmg = false;
    bool spread = false;
    float fireCd = 0.0f;

    while (!WindowShouldClose()) {
        float dt = fminf(GetFrameTime(), 0.05f);
        fireCd = fmaxf(0.0f, fireCd - dt);

        if (IsKeyPressed(KEY_D)) doubleDmg = !doubleDmg;
        if (IsKeyPressed(KEY_S)) spread = !spread;

        // Hold [1] or Left Mouse to repeat gunshots at game fireRate (0.13s)
        if ((IsKeyDown(KEY_ONE) || IsMouseButtonDown(MOUSE_BUTTON_LEFT)) && fireCd <= 0.0f) {
            fireCd = 0.13f;
            play_shot(doubleDmg, spread);
        }
        if (IsKeyPressed(KEY_TWO)) {
            spawn_voice((Voice){ .kind = W_PULSE25, .filt = F_LOWPASS, .filtF = 480.0f, .f = 115.0f, .f2 = 74.0f, .dur = 0.45f, .vol = 0.45f, .wet = 0.35f });
        }
        if (IsKeyPressed(KEY_THREE)) {
            spawn_voice((Voice){ .kind = W_NOISE_LONG, .filt = F_BANDPASS, .filtF = 4200.0f, .dur = 0.12f, .vol = 0.40f, .wet = 0.35f });
            spawn_voice((Voice){ .kind = W_TRI, .f = 2400.0f, .f2 = 1800.0f, .dur = 0.18f, .vol = 0.25f, .wet = 0.40f });
        }
        if (IsKeyPressed(KEY_FOUR)) {
            spawn_voice((Voice){ .kind = W_PULSE25, .filt = F_LOWPASS, .filtF = 1600.0f, .f = 293.66f, .dur = 0.35f, .vol = 0.25f, .wet = 0.45f });
            spawn_voice((Voice){ .kind = W_PULSE25, .filt = F_LOWPASS, .filtF = 1600.0f, .f = 349.23f, .delay = 0.06f, .dur = 0.35f, .vol = 0.22f, .wet = 0.45f });
            spawn_voice((Voice){ .kind = W_TRI, .filt = F_LOWPASS, .filtF = 900.0f, .f = 146.83f, .dur = 0.45f, .vol = 0.35f, .wet = 0.30f });
        }

        BeginDrawing();
        ClearBackground((Color){ 12, 14, 20, 255 });
        DrawText("Procedural AudioStream Synth (44100 Hz, dotted-8th echo, zero .wav files)", 20, 20, 22, GOLD);
        DrawText("Hold [1] or Left Click: Rapid-Fire Gunshot (0.13s fireRate)", 20, 68, 20, RAYWHITE);
        DrawText("[2] Zombie Groan   [3] Glass Shatter   [4] D-Minor Arpeggio", 20, 100, 20, RAYWHITE);
        DrawText(TextFormat("[D] Double Damage (hot 850 Hz): %s", doubleDmg ? "ON" : "OFF"),
                 20, 152, 20, doubleDmg ? (Color){ 255, 140, 50, 255 } : LIGHTGRAY);
        DrawText(TextFormat("[S] Buckshot Spread Layer: %s", spread ? "ON" : "OFF"),
                 20, 184, 20, spread ? (Color){ 232, 210, 138, 255 } : LIGHTGRAY);
        EndDrawing();
    }

    StopAudioStream(stream);
    UnloadAudioStream(stream);
    CloseAudioDevice();
    pthread_mutex_destroy(&gMutex);
    CloseWindow();
    return 0;
}
