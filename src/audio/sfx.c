#include "sfx.h"
#include "synth.h"
#include "music.h"
#include "core/math_util.h"
#include "entities/player.h"
#include <math.h>
#include <string.h>

static float gLastHitT = -1.0f;
static float gLastHoverT = -1.0f;
static bool gStepAlt = false;

static void place_3d(Vector3 pos, float *pan, float *nearVal) {
    float dx = pos.x - gPlayer.position.x;
    float dz = pos.z - gPlayer.position.z;
    *pan = clampf(dx / 12.0f, -0.9f, 0.9f);
    *nearVal = 1.0f / (1.0f + hypotf(dx, dz) / 7.0f);
}

void sfx_shot(bool hot, int spread) {
    float v = 0.65f;
    synth_spawn(&(VoiceParams){
        .type = WAVE_PULSE25, .bus = BUS_SFX, .f = hot ? 850.0f : 680.0f, .f2 = 45.0f,
        .dur = 0.11f, .attack = 0.001f, .vol = 0.85f * v, .wet = 0.22f
    });
    synth_spawn(&(VoiceParams){
        .type = WAVE_NOISE_METAL, .bus = BUS_SFX, .filt = FILT_BANDPASS, .filtF = 3800.0f, .filtQ = 1.0f,
        .dur = 0.06f, .attack = 0.001f, .vol = 0.9f * v, .wet = 0.22f
    });
    synth_spawn(&(VoiceParams){
        .type = WAVE_NOISE_LONG, .bus = BUS_SFX, .rate = 0.7f, .filt = FILT_LOWPASS, .filtF = 2400.0f, .filtF2 = 300.0f,
        .dur = 0.18f, .attack = 0.001f, .vol = 0.8f * v, .wet = 0.22f
    });
    if (spread > 0) {
        synth_spawn(&(VoiceParams){
            .type = WAVE_NOISE_LONG, .bus = BUS_SFX, .delay = 0.01f, .rate = 0.5f, .filt = FILT_LOWPASS, .filtF = 1800.0f,
            .dur = 0.22f, .attack = 0.001f, .vol = 0.5f * v, .wet = 0.22f
        });
    }
    synth_spawn(&(VoiceParams){
        .type = WAVE_SINE, .bus = BUS_SFX, .f = 120.0f, .f2 = 38.0f,
        .dur = 0.14f, .attack = 0.002f, .vol = 0.9f * v, .wet = 0.22f
    });
    synth_duck_music(0.75f, 0.08f);
}

void sfx_hit(Vector3 pos) {
    float now = (float)GetTime();
    if (now - gLastHitT < 0.03f) return;
    gLastHitT = now;
    float pan, nearVal;
    place_3d(pos, &pan, &nearVal);
    float v = 0.55f * nearVal;
    synth_spawn(&(VoiceParams){
        .type = WAVE_NOISE_LONG, .bus = BUS_SFX, .rate = 0.85f, .filt = FILT_BANDPASS, .filtF = 1400.0f, .filtF2 = 350.0f, .filtQ = 2.0f,
        .dur = 0.13f, .attack = 0.001f, .vol = 0.9f * v, .pan = pan, .wet = 0.18f
    });
    synth_spawn(&(VoiceParams){
        .type = WAVE_TRI, .bus = BUS_SFX, .f = 180.0f, .f2 = 55.0f,
        .dur = 0.08f, .attack = 0.002f, .vol = 0.6f * v, .pan = pan, .wet = 0.18f
    });
    synth_spawn(&(VoiceParams){
        .type = WAVE_NOISE_METAL, .bus = BUS_SFX, .filt = FILT_HIGHPASS, .filtF = 4500.0f,
        .dur = 0.04f, .attack = 0.001f, .vol = 0.35f * v, .pan = pan, .wet = 0.18f
    });
}

void sfx_zdeath(Vector3 pos) {
    float pan, nearVal;
    place_3d(pos, &pan, &nearVal);
    float v = 0.7f * nearVal;
    synth_spawn(&(VoiceParams){
        .type = WAVE_NOISE_LONG, .bus = BUS_SFX, .rate = 0.5f, .filt = FILT_LOWPASS, .filtF = 1800.0f, .filtF2 = 200.0f,
        .dur = 0.32f, .attack = 0.001f, .vol = 1.0f * v, .pan = pan, .wet = 0.35f
    });
    synth_spawn(&(VoiceParams){
        .type = WAVE_TRI, .bus = BUS_SFX, .f = 140.0f, .f2 = 35.0f,
        .dur = 0.22f, .attack = 0.002f, .vol = 0.9f * v, .pan = pan, .wet = 0.35f
    });
    synth_spawn(&(VoiceParams){
        .type = WAVE_NOISE_METAL, .bus = BUS_SFX, .delay = 0.04f, .rate = 0.6f, .filt = FILT_BANDPASS, .filtF = 2200.0f, .filtQ = 1.5f,
        .dur = 0.15f, .attack = 0.001f, .vol = 0.5f * v, .pan = pan, .wet = 0.35f
    });
    synth_spawn(&(VoiceParams){
        .type = WAVE_GROWL, .bus = BUS_SFX, .f = rndf(65.0f, 85.0f), .f2 = 40.0f,
        .dur = 0.35f, .attack = 0.08f, .vol = 0.45f * v, .pan = pan, .wet = 0.35f, .formant = 420.0f
    });
}

void sfx_groan(Vector3 pos) {
    float pan, nearVal;
    place_3d(pos, &pan, &nearVal);
    float f = rndf(55.0f, 95.0f);
    synth_spawn(&(VoiceParams){
        .type = WAVE_GROWL, .bus = BUS_SFX, .f = f, .f2 = f * rndf(0.7f, 0.95f),
        .dur = rndf(0.7f, 1.4f), .attack = 0.08f, .vol = 0.4f * nearVal * rndf(0.35f, 0.6f),
        .pan = pan, .wet = 0.4f, .formant = rndf(450.0f, 750.0f)
    });
}

void sfx_swipe(Vector3 pos) {
    float pan, nearVal;
    place_3d(pos, &pan, &nearVal);
    synth_spawn(&(VoiceParams){
        .type = WAVE_NOISE_WHITE, .bus = BUS_SFX, .filt = FILT_BANDPASS, .filtF = 1800.0f, .filtF2 = 400.0f, .filtQ = 1.8f,
        .dur = 0.18f, .attack = 0.001f, .vol = 0.45f * nearVal * 0.8f, .pan = pan, .wet = 0.1f
    });
}

void sfx_bash(Vector3 pos, float scale) {
    float pan, nearVal;
    place_3d(pos, &pan, &nearVal);
    float v = 0.6f * nearVal * scale;
    synth_spawn(&(VoiceParams){
        .type = WAVE_TRI, .bus = BUS_SFX, .f = 110.0f, .f2 = 42.0f,
        .dur = 0.18f, .attack = 0.002f, .vol = 0.9f * v, .pan = pan, .wet = 0.25f
    });
    synth_spawn(&(VoiceParams){
        .type = WAVE_NOISE_LONG, .bus = BUS_SFX, .rate = 0.6f, .filt = FILT_BANDPASS, .filtF = 1100.0f, .filtF2 = 240.0f, .filtQ = 1.2f,
        .dur = 0.2f, .attack = 0.001f, .vol = 0.85f * v, .pan = pan, .wet = 0.25f
    });
    synth_spawn(&(VoiceParams){
        .type = WAVE_NOISE_METAL, .bus = BUS_SFX, .filt = FILT_HIGHPASS, .filtF = 3000.0f,
        .dur = 0.05f, .attack = 0.001f, .vol = 0.35f * v, .pan = pan, .wet = 0.25f
    });
}

void sfx_crack(Vector3 pos) {
    float pan, nearVal;
    place_3d(pos, &pan, &nearVal);
    float v = 0.6f * nearVal;
    synth_spawn(&(VoiceParams){
        .type = WAVE_PULSE12, .bus = BUS_SFX, .f = 2200.0f, .f2 = 900.0f,
        .dur = 0.04f, .attack = 0.001f, .vol = 0.5f * v, .pan = pan, .wet = 0.25f
    });
    synth_spawn(&(VoiceParams){
        .type = WAVE_NOISE_METAL, .bus = BUS_SFX, .filt = FILT_HIGHPASS, .filtF = 6500.0f,
        .dur = 0.12f, .attack = 0.001f, .vol = 0.7f * v, .pan = pan, .wet = 0.25f
    });
    synth_spawn(&(VoiceParams){
        .type = WAVE_NOISE_WHITE, .bus = BUS_SFX, .filt = FILT_BANDPASS, .filtF = 4200.0f, .filtQ = 2.0f,
        .dur = 0.08f, .attack = 0.001f, .vol = 0.5f * v, .pan = pan, .wet = 0.25f
    });
}

void sfx_shatter(Vector3 pos) {
    float pan, nearVal;
    place_3d(pos, &pan, &nearVal);
    float v = 0.75f * nearVal;
    synth_spawn(&(VoiceParams){
        .type = WAVE_NOISE_METAL, .bus = BUS_SFX, .filt = FILT_HIGHPASS, .filtF = 4500.0f,
        .dur = 0.45f, .attack = 0.001f, .vol = 0.9f * v, .pan = pan, .wet = 0.4f
    });
    synth_spawn(&(VoiceParams){
        .type = WAVE_NOISE_WHITE, .bus = BUS_SFX, .filt = FILT_BANDPASS, .filtF = 2600.0f, .filtQ = 1.5f,
        .dur = 0.3f, .attack = 0.001f, .vol = 0.7f * v, .pan = pan, .wet = 0.4f
    });
    synth_spawn(&(VoiceParams){
        .type = WAVE_TRI, .bus = BUS_SFX, .f = 160.0f, .f2 = 50.0f,
        .dur = 0.16f, .attack = 0.002f, .vol = 0.7f * v, .pan = pan, .wet = 0.4f
    });
    for (int i = 0; i < 4; i++) {
        synth_spawn(&(VoiceParams){
            .type = WAVE_PULSE12, .bus = BUS_SFX, .delay = 0.04f + i * 0.06f, .f = rndf(1800.0f, 3400.0f),
            .dur = 0.03f, .attack = 0.001f, .vol = 0.25f * v, .pan = pan, .wet = 0.4f
        });
    }
}

void sfx_door_break(Vector3 pos) {
    float pan, nearVal;
    place_3d(pos, &pan, &nearVal);
    float v = 0.85f * nearVal;
    synth_spawn(&(VoiceParams){
        .type = WAVE_SINE, .bus = BUS_SFX, .f = 90.0f, .f2 = 32.0f,
        .dur = 0.35f, .attack = 0.002f, .vol = 1.0f * v, .pan = pan, .wet = 0.45f
    });
    synth_spawn(&(VoiceParams){
        .type = WAVE_NOISE_LONG, .bus = BUS_SFX, .rate = 0.4f, .filt = FILT_LOWPASS, .filtF = 900.0f,
        .dur = 0.5f, .attack = 0.001f, .vol = 1.0f * v, .pan = pan, .wet = 0.45f
    });
    synth_spawn(&(VoiceParams){
        .type = WAVE_NOISE_METAL, .bus = BUS_SFX, .filt = FILT_BANDPASS, .filtF = 1800.0f, .filtQ = 1.2f,
        .dur = 0.35f, .attack = 0.001f, .vol = 0.8f * v, .pan = pan, .wet = 0.45f
    });
}

void sfx_step(void) {
    gStepAlt = !gStepAlt;
    float v = 0.2f;
    synth_spawn(&(VoiceParams){
        .type = WAVE_TRI, .bus = BUS_SFX, .f = gStepAlt ? 130.0f : 115.0f, .f2 = 55.0f,
        .dur = 0.05f, .attack = 0.002f, .vol = 0.45f * v, .wet = 0.08f
    });
    synth_spawn(&(VoiceParams){
        .type = WAVE_NOISE_LONG, .bus = BUS_SFX, .rate = 0.9f, .filt = FILT_LOWPASS, .filtF = 1200.0f,
        .dur = 0.04f, .attack = 0.001f, .vol = 0.3f * v, .wet = 0.08f
    });
}

void sfx_reload_out(void) {
    float v = 0.4f;
    synth_spawn(&(VoiceParams){
        .type = WAVE_PULSE25, .bus = BUS_SFX, .f = 720.0f, .f2 = 340.0f,
        .dur = 0.05f, .attack = 0.002f, .vol = 0.35f * v, .pan = 0.1f, .wet = 0.1f
    });
    synth_spawn(&(VoiceParams){
        .type = WAVE_NOISE_METAL, .bus = BUS_SFX, .filt = FILT_BANDPASS, .filtF = 2400.0f,
        .dur = 0.07f, .attack = 0.001f, .vol = 0.5f * v, .pan = 0.1f, .wet = 0.1f
    });
}

void sfx_reload_in(void) {
    float v = 0.55f;
    synth_spawn(&(VoiceParams){
        .type = WAVE_PULSE25, .bus = BUS_SFX, .f = 420.0f, .f2 = 880.0f,
        .dur = 0.06f, .attack = 0.002f, .vol = 0.5f * v, .pan = 0.05f, .wet = 0.12f
    });
    synth_spawn(&(VoiceParams){
        .type = WAVE_NOISE_METAL, .bus = BUS_SFX, .delay = 0.02f, .filt = FILT_HIGHPASS, .filtF = 3200.0f,
        .dur = 0.09f, .attack = 0.001f, .vol = 0.6f * v, .pan = 0.05f, .wet = 0.12f
    });
    synth_spawn(&(VoiceParams){
        .type = WAVE_TRI, .bus = BUS_SFX, .delay = 0.06f, .f = 210.0f, .f2 = 95.0f,
        .dur = 0.06f, .attack = 0.002f, .vol = 0.4f * v, .pan = 0.05f, .wet = 0.12f
    });
}

void sfx_hurt(void) {
    float v = 0.9f;
    synth_spawn(&(VoiceParams){
        .type = WAVE_SINE, .bus = BUS_POST, .f = 85.0f, .f2 = 30.0f,
        .dur = 0.35f, .attack = 0.002f, .vol = 1.0f * v, .wet = 0.35f
    });
    synth_spawn(&(VoiceParams){
        .type = WAVE_NOISE_LONG, .bus = BUS_POST, .rate = 0.4f, .filt = FILT_LOWPASS, .filtF = 600.0f,
        .dur = 0.25f, .attack = 0.001f, .vol = 0.9f * v, .wet = 0.35f
    });
    synth_muffle(1.2f, 550.0f);
    synth_duck_music(0.15f, 0.9f);
}

void sfx_pickup(int kind) {
    const float notes[3][4] = {
        { 60, 64, 67, 72 },
        { 58, 62, 65, 70 },
        { 55, 60, 67, 72 }
    };
    int k = (kind >= 0 && kind < 3) ? kind : 2;
    for (int i = 0; i < 4; i++) {
        float d = (float)i * 0.055f;
        synth_spawn(&(VoiceParams){
            .type = WAVE_PULSE25, .bus = BUS_SFX, .delay = d, .f = midi_hz(notes[k][i]),
            .dur = 0.18f, .attack = 0.002f, .vol = 0.15f, .wet = 0.3f
        });
        synth_spawn(&(VoiceParams){
            .type = WAVE_TRI, .bus = BUS_SFX, .delay = d, .f = midi_hz(notes[k][i] - 12.0f),
            .dur = 0.12f, .attack = 0.002f, .vol = 0.125f, .wet = 0.3f
        });
    }
    synth_spawn(&(VoiceParams){
        .type = WAVE_NOISE_METAL, .bus = BUS_SFX, .filt = FILT_HIGHPASS, .filtF = 6000.0f,
        .dur = 0.4f, .attack = 0.001f, .vol = 0.04f, .wet = 0.3f
    });
}

void sfx_wave_start(int n) {
    music_set_mode(MUSIC_COMBAT);
    const float partials[5][2] = { {1.0f, 1.0f}, {2.01f, 0.5f}, {2.76f, 0.4f}, {5.4f, 0.2f}, {8.93f, 0.1f} };
    for (int i = 0; i < 5; i++) {
        synth_spawn(&(VoiceParams){
            .type = WAVE_SINE, .bus = BUS_SFX, .f = midi_hz(50.0f) * partials[i][0],
            .dur = 3.2f / sqrtf(partials[i][0]), .attack = 0.005f, .vol = partials[i][1] * 0.3f, .wet = 0.6f
        });
    }
    synth_spawn(&(VoiceParams){
        .type = WAVE_TRI, .bus = BUS_SFX, .f = midi_hz(38.0f), .dur = 1.6f, .attack = 0.002f, .vol = 0.3f, .wet = 0.6f
    });
    if (n > 1) {
        const float omen[3] = { 57, 58, 57 };
        for (int i = 0; i < 3; i++) {
            synth_spawn(&(VoiceParams){
                .type = WAVE_PULSE25, .bus = BUS_SFX, .delay = 0.12f + i * 0.22f,
                .f = midi_hz(omen[i]), .hold = 0.18f, .attack = 0.005f, .vol = 0.11f, .wet = 0.6f
            });
        }
    }
    synth_duck_music(0.35f, 1.2f);
}

void sfx_wave_clear(void) {
    music_set_mode(MUSIC_CALM);
    const float fanfare[4] = { 62, 66, 69, 74 };
    for (int i = 0; i < 4; i++) {
        synth_spawn(&(VoiceParams){
            .type = WAVE_PULSE25, .bus = BUS_SFX, .delay = i * 0.09f, .f = midi_hz(fanfare[i]),
            .hold = 0.08f, .attack = 0.004f, .vol = 0.165f, .wet = 0.35f
        });
    }
    synth_spawn(&(VoiceParams){
        .type = WAVE_PULSE25, .bus = BUS_SFX, .delay = 0.36f, .f = midi_hz(78.0f),
        .hold = 0.7f, .attack = 0.005f, .vol = 0.165f, .vib = 0.3f, .wet = 0.35f
    });
    synth_spawn(&(VoiceParams){
        .type = WAVE_PULSE12, .bus = BUS_SFX, .delay = 0.36f, .f = midi_hz(74.0f),
        .hold = 0.7f, .attack = 0.005f, .vol = 0.08f, .wet = 0.35f
    });
    synth_spawn(&(VoiceParams){
        .type = WAVE_TRI, .bus = BUS_SFX, .delay = 0.36f, .f = midi_hz(50.0f),
        .hold = 1.0f, .attack = 0.005f, .vol = 0.275f, .wet = 0.35f
    });
    synth_spawn(&(VoiceParams){
        .type = WAVE_NOISE_METAL, .bus = BUS_SFX, .delay = 0.36f, .filt = FILT_HIGHPASS, .filtF = 5000.0f,
        .dur = 1.0f, .attack = 0.002f, .vol = 0.066f, .wet = 0.35f
    });
    synth_duck_music(0.0f, 1.4f);
}

void sfx_deal(int count) {
    for (int i = 0; i < count; i++) {
        float pan = ((float)i - (float)(count - 1) * 0.5f) * 0.5f;
        float d = 0.05f + i * 0.09f;
        synth_spawn(&(VoiceParams){
            .type = WAVE_NOISE_WHITE, .bus = BUS_SFX, .delay = d, .filt = FILT_BANDPASS, .filtF = 3200.0f, .filtF2 = 1400.0f, .filtQ = 1.5f,
            .dur = 0.07f, .attack = 0.001f, .vol = 0.28f, .pan = pan, .wet = 0.1f
        });
        synth_spawn(&(VoiceParams){
            .type = WAVE_PULSE12, .bus = BUS_SFX, .delay = d, .f = midi_hz(74.0f + i * 5.0f),
            .dur = 0.05f, .attack = 0.002f, .vol = 0.048f, .pan = pan, .wet = 0.1f
        });
    }
}

void sfx_hover(int idx) {
    float now = (float)GetTime();
    if (now - gLastHoverT < 0.05f) return;
    gLastHoverT = now;
    synth_spawn(&(VoiceParams){
        .type = WAVE_PULSE12, .bus = BUS_SFX, .f = midi_hz(81.0f + idx * 2.0f),
        .dur = 0.04f, .attack = 0.002f, .vol = 0.075f, .pan = (float)(idx - 1) * 0.4f, .wet = 0.1f
    });
}

void sfx_upgrade(int catIdx) {
    const float up[4] = { 0, 2, 5, 7 };
    float shift = (catIdx >= 0 && catIdx < 4) ? up[catIdx] : 0.0f;
    synth_spawn(&(VoiceParams){
        .type = WAVE_PULSE12, .bus = BUS_SFX, .f = 180.0f, .f2 = 1700.0f,
        .dur = 0.3f, .attack = 0.002f, .vol = 0.066f, .wet = 0.35f
    });
    const float notes[5] = { 62, 69, 74, 78, 81 };
    for (int i = 0; i < 5; i++) {
        synth_spawn(&(VoiceParams){
            .type = WAVE_PULSE25, .bus = BUS_SFX, .delay = i * 0.05f, .f = midi_hz(notes[i] + shift),
            .dur = 0.2f, .attack = 0.002f, .vol = 0.138f, .wet = 0.35f
        });
    }
}

void sfx_death(void) {
    music_set_mode(MUSIC_DEAD);
    float d0 = 0.35f;
    const float top[3] = { 69, 68, 67 };
    for (int i = 0; i < 3; i++) {
        synth_spawn(&(VoiceParams){
            .type = WAVE_PULSE25, .bus = BUS_POST, .delay = d0 + i * 0.32f, .f = midi_hz(top[i]),
            .hold = 0.26f, .attack = 0.005f, .vol = 0.168f, .wet = 0.45f
        });
    }
    synth_spawn(&(VoiceParams){
        .type = WAVE_PULSE25, .bus = BUS_POST, .delay = d0 + 0.96f, .f = midi_hz(66.0f), .f2 = midi_hz(61.0f),
        .hold = 1.3f, .attack = 0.005f, .vol = 0.168f, .vib = 0.5f, .wet = 0.45f
    });
    const float bot[4] = { 57, 56, 55, 50 };
    for (int i = 0; i < 4; i++) {
        synth_spawn(&(VoiceParams){
            .type = WAVE_TRI, .bus = BUS_POST, .delay = d0 + i * 0.32f, .f = midi_hz(bot[i] - 12.0f),
            .hold = i < 3 ? 0.26f : 1.6f, .attack = 0.005f, .vol = 0.3f, .wet = 0.45f
        });
    }
    const float bell[3][2] = { {1.0f, 1.0f}, {2.76f, 0.4f}, {5.4f, 0.2f} };
    for (int i = 0; i < 3; i++) {
        synth_spawn(&(VoiceParams){
            .type = WAVE_SINE, .bus = BUS_POST, .delay = d0 + 0.96f, .f = midi_hz(38.0f) * bell[i][0],
            .dur = 3.0f, .attack = 0.005f, .vol = bell[i][1] * 0.21f, .wet = 0.45f
        });
    }
}

void sfx_thunder(float nearVal) {
    float v = 0.45f + 0.45f * nearVal;
    float pan = rndf(-0.4f, 0.4f);
    for (int i = 0; i < 4; i++) {
        synth_spawn(&(VoiceParams){
            .type = WAVE_NOISE_LONG, .bus = BUS_SFX, .delay = i * rndf(0.2f, 0.6f), .rate = 0.08f,
            .filt = FILT_LOWPASS, .filtF = rndf(140.0f, 320.0f),
            .dur = rndf(1.8f, 4.2f), .attack = rndf(0.05f, 0.3f), .vol = rndf(0.5f, 0.9f) * v,
            .pan = pan, .wet = 0.5f
        });
    }
    synth_duck_music(0.55f, 1.8f);
}

void sfx_ui(const char *kind) {
    float now = (float)GetTime();
    if (now - gLastHoverT < 0.04f) return;
    gLastHoverT = now;
    if (strcmp(kind, "move") == 0) {
        synth_spawn(&(VoiceParams){
            .type = WAVE_PULSE12, .bus = BUS_SFX, .f = midi_hz(81.0f),
            .dur = 0.035f, .attack = 0.002f, .vol = 0.06f, .wet = 0.12f
        });
    } else if (strcmp(kind, "back") == 0) {
        synth_spawn(&(VoiceParams){
            .type = WAVE_PULSE12, .bus = BUS_SFX, .f = midi_hz(74.0f), .f2 = midi_hz(69.0f),
            .dur = 0.07f, .attack = 0.002f, .vol = 0.054f, .wet = 0.12f
        });
    } else {
        synth_spawn(&(VoiceParams){
            .type = WAVE_PULSE12, .bus = BUS_SFX, .f = midi_hz(76.0f),
            .dur = 0.06f, .attack = 0.002f, .vol = 0.054f, .wet = 0.12f
        });
        synth_spawn(&(VoiceParams){
            .type = WAVE_PULSE12, .bus = BUS_SFX, .delay = 0.05f, .f = midi_hz(83.0f),
            .dur = 0.06f, .attack = 0.002f, .vol = 0.054f, .wet = 0.12f
        });
    }
}
