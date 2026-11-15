// Lesson 8: Procedural AudioStream synth (band-limited pulse/tri, LFSR noise, SVF, recursive mutex)
#include "raylib.h"
#include <math.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#define SR 44100
#define MAX_V 16

typedef enum { W_TRI = 0, W_PULSE25, W_NOISE } WaveKind;
typedef struct {
    bool active;
    WaveKind kind;
    float f, f2, phase, t, dur, vol;
    float lp, bp, filtF;
} Voice;

static Voice gVoices[MAX_V];
static float gNoise[SR];
static pthread_mutex_t gMutex;

static void spawn_voice(WaveKind kind, float f, float f2, float dur, float vol, float filtF) {
    pthread_mutex_lock(&gMutex);
    for (int i = 0; i < MAX_V; i++) {
        if (!gVoices[i].active) {
            gVoices[i] = (Voice){ .active = true, .kind = kind, .f = f, .f2 = f2, .dur = dur, .vol = vol, .filtF = filtF };
            break;
        }
    }
    pthread_mutex_unlock(&gMutex);
}

static void audio_cb(void *buf, unsigned int frames) {
    short *out = (short *)buf;
    const float dt = 1.0f / (float)SR;
    pthread_mutex_lock(&gMutex);
    for (unsigned int s = 0; s < frames; s++) {
        float mix = 0.0f;
        for (int i = 0; i < MAX_V; i++) {
            Voice *v = &gVoices[i];
            if (!v->active) continue;
            float u = v->t / v->dur;
            if (u >= 1.0f) { v->active = false; continue; }
            float freq = v->f2 > 0.0f ? v->f * powf(v->f2 / v->f, u) : v->f;
            v->phase = fmodf(v->phase + freq * dt, 1.0f);

            float raw = 0.0f;
            if (v->kind == W_TRI) raw = 1.0f - 4.0f * fabsf(v->phase - 0.5f);
            else if (v->kind == W_PULSE25) raw = (v->phase < 0.25f) ? 1.0f : -1.0f;
            else raw = gNoise[((int)(v->t * (float)SR)) % SR];

            if (v->filtF > 0.0f) {
                float fc = fminf(0.45f, 2.0f * sinf(PI * v->filtF * dt));
                v->lp += fc * v->bp;
                float hp = raw - v->lp - 0.7f * v->bp;
                v->bp += fc * hp;
                raw = v->lp;
            }
            mix += raw * expf(-u * 5.0f) * v->vol;
            v->t += dt;
        }
        float lim = tanhf(mix);
        short sample = (short)(lim * 26000.0f);
        out[s * 2 + 0] = sample;
        out[s * 2 + 1] = sample;
    }
    pthread_mutex_unlock(&gMutex);
}

int main(void) {
    InitWindow(960, 540, "Ex 08: Procedural AudioStream Synth");
    SetTargetFPS(60);

    // 15-bit LFSR noise
    uint16_t lfsr = 1;
    for (int i = 0; i < SR; i++) {
        uint16_t bit = (uint16_t)((lfsr ^ (lfsr >> 1)) & 1u);
        lfsr = (uint16_t)((lfsr >> 1) | (bit << 14));
        gNoise[i] = (lfsr & 1u) ? 1.0f : -1.0f;
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

    while (!WindowShouldClose()) {
        if (IsKeyPressed(KEY_ONE)) { // Gunshot
            spawn_voice(W_NOISE, 1.0f, 0.0f, 0.16f, 0.55f, 1800.0f);
            spawn_voice(W_TRI, 190.0f, 48.0f, 0.11f, 0.55f, 0.0f);
        }
        if (IsKeyPressed(KEY_TWO)) { // Zombie groan
            spawn_voice(W_PULSE25, 115.0f, 74.0f, 0.45f, 0.40f, 480.0f);
        }
        if (IsKeyPressed(KEY_THREE)) { // Glass chime
            spawn_voice(W_NOISE, 1.0f, 0.0f, 0.12f, 0.35f, 4200.0f);
            spawn_voice(W_TRI, 2400.0f, 1800.0f, 0.18f, 0.25f, 0.0f);
        }
        if (IsKeyPressed(KEY_FOUR)) { // D-minor arpeggio chord
            spawn_voice(W_PULSE25, 293.66f, 0.0f, 0.35f, 0.25f, 1600.0f);
            spawn_voice(W_PULSE25, 349.23f, 0.0f, 0.35f, 0.22f, 1600.0f);
            spawn_voice(W_TRI, 146.83f, 0.0f, 0.45f, 0.35f, 900.0f);
        }

        BeginDrawing();
        ClearBackground((Color){ 12, 14, 20, 255 });
        DrawText("Procedural AudioStream Synth (44100 Hz, zero .wav files)", 20, 20, 22, GOLD);
        DrawText("[1] Gunshot   [2] Zombie Groan   [3] Glass   [4] D-Minor Chord", 20, 64, 20, RAYWHITE);
        EndDrawing();
    }

    StopAudioStream(stream);
    UnloadAudioStream(stream);
    CloseAudioDevice();
    pthread_mutex_destroy(&gMutex);
    CloseWindow();
    return 0;
}
