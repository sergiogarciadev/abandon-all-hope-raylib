#include "music.h"
#include "synth.h"
#include "core/math_util.h"
#include <math.h>
#include <stddef.h>

typedef enum {
    CH_DM9 = 0, CH_DMADD9, CH_BBMAJ9, CH_BBMAJ7, CH_BBMAJ7S11,
    CH_GM9, CH_GM7, CH_EO7, CH_A7, CH_A7B9,
    CH_FMAJ7, CH_FMAJ9, CH_CADD9, CH_C69, CH_C7SUS,
    CH_C7, CH_AM7, CH_EBMAJ7S11
} ChordId;

static const int CHORDS[18][5] = {
    {38, 53, 57, 60, 64}, // Dm9
    {38, 53, 57, 62, 64}, // Dmadd9
    {34, 53, 57, 60, 62}, // Bbmaj9
    {34, 53, 58, 62, 69}, // Bbmaj7
    {34, 52, 57, 62, 65}, // Bbmaj7s11
    {43, 53, 58, 62, 69}, // Gm9
    {43, 53, 58, 62, 67}, // Gm7
    {40, 52, 55, 58, 62}, // Eo7
    {45, 55, 61, 64, 69}, // A7
    {45, 55, 61, 64, 70}, // A7b9
    {41, 53, 57, 60, 64}, // Fmaj7
    {41, 55, 57, 60, 64}, // Fmaj9
    {36, 55, 60, 62, 64}, // Cadd9
    {36, 55, 57, 62, 64}, // C69
    {36, 53, 55, 58, 60}, // C7sus
    {36, 52, 55, 58, 60}, // C7
    {45, 55, 60, 64, 69}, // Am7
    {39, 55, 58, 62, 69}, // Ebmaj7s11
};

typedef struct { int step, m, l; } MelNote;

static const MelNote THEME_NOTES[] = {
    {0,69,4},{4,74,2},{6,77,2},{8,76,6},{14,74,2},
    {16,77,4},{20,74,4},{24,70,8},
    {32,67,2},{34,70,2},{36,74,4},{40,79,6},{46,77,2},
    {48,76,4},{52,73,4},{56,69,8},
    {64,69,4},{68,74,2},{70,77,2},{72,81,6},{78,79,2},
    {80,77,4},{84,81,4},{88,84,4},{92,81,4},
    {96,79,4},{100,77,2},{102,74,2},{104,70,4},{108,74,4},
    {112,73,4},{116,76,4},{120,81,4},{124,73,4}
};
static const MelNote SOAR_NOTES[] = {
    {0,74,6},{6,72,2},{8,74,4},{12,77,4},
    {16,76,8},{24,74,4},{28,72,4},
    {32,72,4},{36,76,4},{40,81,8},
    {48,79,4},{52,77,4},{56,76,4},{60,74,4},
    {64,74,6},{70,72,2},{72,74,4},{76,81,4},
    {80,79,8},{88,76,4},{92,79,4},
    {96,82,8},{104,81,4},{108,79,4},
    {112,82,4},{116,81,12}
};
static const MelNote OMEN_NOTES[] = {
    {0,81,12},{12,82,4},{16,81,16},
    {32,79,6},{38,77,2},{40,74,8},{48,76,16},
    {64,81,12},{76,82,4},{80,81,16},
    {96,82,8},{104,81,4},{108,79,4},
    {112,73,4},{116,76,4},{120,79,4},{124,82,4}
};
static const MelNote HAVEN_NOTES[] = {
    {0,76,6},{6,74,2},{8,72,8},
    {16,72,4},{20,76,4},{24,79,8},
    {32,77,6},{38,76,2},{40,74,8},
    {48,72,4},{52,77,4},{56,76,8},
    {64,81,6},{70,79,2},{72,77,8},
    {80,76,4},{84,77,4},{88,81,8},
    {96,82,6},{102,81,2},{104,79,4},{108,77,4},
    {112,76,4},{116,73,4},{120,70,8}
};

typedef struct {
    ChordId bars[8][2];
    int barLen[8];
    const MelNote *mel;
    int melCount;
    int pedal;
} Section;

static const Section SECTIONS[5] = {
    // A
    {
        .bars = { {CH_DM9,0},{CH_BBMAJ9,0},{CH_GM9,0},{CH_A7,CH_A7B9},{CH_DMADD9,0},{CH_FMAJ7,0},{CH_GM7,CH_EO7},{CH_A7B9,0} },
        .barLen = {1,1,1,2,1,1,2,1},
        .mel = THEME_NOTES, .melCount = sizeof(THEME_NOTES)/sizeof(THEME_NOTES[0]), .pedal = 0
    },
    // B
    {
        .bars = { {CH_BBMAJ7,0},{CH_CADD9,0},{CH_AM7,0},{CH_DM9,0},{CH_BBMAJ7,0},{CH_C69,0},{CH_EBMAJ7S11,0},{CH_A7B9,0} },
        .barLen = {1,1,1,1,1,1,1,1},
        .mel = SOAR_NOTES, .melCount = sizeof(SOAR_NOTES)/sizeof(SOAR_NOTES[0]), .pedal = 0
    },
    // C
    {
        .bars = { {CH_DMADD9,0},{CH_BBMAJ7S11,0},{CH_GM9,0},{CH_DM9,0},{CH_DMADD9,0},{CH_EBMAJ7S11,0},{CH_GM7,0},{CH_A7B9,0} },
        .barLen = {1,1,1,1,1,1,1,1},
        .mel = OMEN_NOTES, .melCount = sizeof(OMEN_NOTES)/sizeof(OMEN_NOTES[0]), .pedal = 38
    },
    // I (title interlude)
    {
        .bars = { {CH_BBMAJ7,0},{CH_GM9,0},{CH_EBMAJ7S11,0},{CH_DMADD9,0},{CH_BBMAJ7S11,0},{CH_GM9,0},{CH_EO7,0},{CH_A7B9,0} },
        .barLen = {1,1,1,1,1,1,1,1},
        .mel = NULL, .melCount = 0, .pedal = 0
    },
    // F (calm)
    {
        .bars = { {CH_FMAJ9,0},{CH_AM7,0},{CH_BBMAJ7,0},{CH_C7SUS,CH_C7},{CH_FMAJ9,0},{CH_DM9,0},{CH_GM9,0},{CH_A7B9,0} },
        .barLen = {1,1,1,2,1,1,1,1},
        .mel = HAVEN_NOTES, .melCount = sizeof(HAVEN_NOTES)/sizeof(HAVEN_NOTES[0]), .pedal = 0
    }
};

static const int ARP_UPDOWN[16] = { 0, 1, 2, 3, 4, 5, 6, 7, 6, 5, 4, 3, 2, 1, 2, 3 };

static struct {
    MusicMode mode;
    MusicMode pending;
    bool hasPending;
    int step;
    float accum;
    int sec;
    int secN;
    bool vary;
    int layer;
    int wave;
    int alive;
    bool lowHp;
    float reviveT;
} gMus;

static float hum(float v) { return v * rndf(0.9f, 1.1f); }

static float mus_bpm(void) {
    if (gMus.mode == MUSIC_COMBAT) return fminf(156.0f, 126.0f + gMus.wave * 2.0f);
    return gMus.mode == MUSIC_CALM ? 92.0f : 76.0f;
}

static float mus_sixteenth(void) { return 60.0f / mus_bpm() / 4.0f; }

static float mus_heat(void) { return 0.1f + gMus.wave * 0.06f + ((float)gMus.alive / 30.0f) * 0.6f; }

static void apply_filter(void) {
    float f = (gMus.mode == MUSIC_DEAD) ? 120.0f
            : gMus.lowHp ? 900.0f
            : (gMus.mode == MUSIC_CALM) ? 6000.0f : 18000.0f;
    synth_set_music_lp(f);
}

static void enter_mode(MusicMode m) {
    gMus.hasPending = false;
    if (m == gMus.mode) return;
    MusicMode prev = gMus.mode;
    gMus.mode = m;
    gMus.step = 0;
    gMus.secN = 0;
    gMus.reviveT = 0.0f;
    if (prev == MUSIC_DEAD) synth_set_music_duck_target(1.0f, 0.8f);
    apply_filter();
}

static void pick_section(void) {
    int n = gMus.secN++;
    if (gMus.mode == MUSIC_TITLE) {
        gMus.sec = (n % 3 == 2) ? 3 : 0;
        gMus.vary = (n % 3 == 1);
    } else if (gMus.mode == MUSIC_CALM) {
        gMus.sec = 4;
        gMus.vary = (n % 2 == 1);
    } else {
        int k = n % 4;
        gMus.sec = (k == 1) ? 1 : (k == 3 ? (mus_heat() >= 0.55f ? 1 : 2) : 0);
        gMus.vary = (k >= 2);
    }
}

static int harmony_under(int m, const int ch[5]) {
    for (int t = m - 3; t >= m - 9; t--) {
        for (int i = 0; i < 5; i++) {
            if (((ch[i] - t) % 12 + 12) % 12 == 0) return t;
        }
    }
    return -1;
}

static void play_pad(float at, const int ch[5], int steps, float v, float a) {
    float len = (float)steps * mus_sixteenth();
    for (int i = 1; i < 5; i++) {
        for (int k = 0; k < 2; k++) {
            synth_spawn(&(VoiceParams){
                .type = k ? WAVE_PULSE50 : WAVE_PULSE25, .bus = BUS_MUSIC_PUMP, .delay = at,
                .f = midi_hz((float)ch[i]), .hold = len, .attack = a, .release = 0.7f,
                .vol = v, .pan = k ? 0.55f : -0.55f, .detune = k ? 8.0f : -8.0f,
                .filt = FILT_LOWPASS, .filtF = 900.0f
            });
        }
    }
}

static void play_bass(float at, int m, int steps, float v) {
    float len = (float)steps * mus_sixteenth() * 0.85f;
    synth_spawn(&(VoiceParams){
        .type = WAVE_TRI, .bus = BUS_MUSIC, .delay = at, .f = midi_hz((float)m),
        .hold = len, .attack = 0.004f, .release = 0.05f, .vol = v
    });
    synth_spawn(&(VoiceParams){
        .type = WAVE_SINE, .bus = BUS_MUSIC, .delay = at, .f = midi_hz((float)m),
        .hold = len, .attack = 0.004f, .release = 0.05f, .vol = v * 0.5f
    });
}

static void play_bell(float at, int m, float v) {
    synth_spawn(&(VoiceParams){
        .type = WAVE_SINE, .bus = BUS_MUSIC, .delay = at, .f = midi_hz((float)m),
        .hold = 0.01f, .attack = 0.002f, .release = 1.8f, .vol = v, .pan = 0.15f, .wet = 0.22f
    });
    synth_spawn(&(VoiceParams){
        .type = WAVE_SINE, .bus = BUS_MUSIC, .delay = at, .f = midi_hz((float)(m + 12)),
        .hold = 0.01f, .attack = 0.002f, .release = 0.5f, .vol = v * 0.3f, .pan = 0.15f, .wet = 0.22f
    });
}

static void play_kick(float at, float v) {
    synth_spawn(&(VoiceParams){
        .type = WAVE_SINE, .bus = BUS_MUSIC, .delay = at, .f = 160.0f, .f2 = 44.0f,
        .dur = 0.18f, .attack = 0.002f, .vol = v
    });
    synth_spawn(&(VoiceParams){
        .type = WAVE_NOISE_WHITE, .bus = BUS_MUSIC, .delay = at, .filt = FILT_HIGHPASS, .filtF = 2500.0f,
        .dur = 0.012f, .attack = 0.001f, .vol = v * 0.22f
    });
    synth_trigger_pump();
}

static void play_snare(float at, float v) {
    synth_spawn(&(VoiceParams){
        .type = WAVE_NOISE_LONG, .bus = BUS_MUSIC, .delay = at, .rate = 0.55f,
        .filt = FILT_BANDPASS, .filtF = 2200.0f, .filtQ = 0.8f, .dur = 0.16f, .attack = 0.001f, .vol = v
    });
    synth_spawn(&(VoiceParams){
        .type = WAVE_TRI, .bus = BUS_MUSIC, .delay = at, .f = 230.0f, .f2 = 150.0f,
        .dur = 0.07f, .attack = 0.002f, .vol = v * 0.6f
    });
}

static void play_hat(float at, float v, bool open) {
    synth_spawn(&(VoiceParams){
        .type = WAVE_NOISE_METAL, .bus = BUS_MUSIC, .delay = at,
        .filt = FILT_HIGHPASS, .filtF = 7000.0f, .dur = open ? 0.2f : 0.035f,
        .attack = 0.001f, .vol = hum(v), .pan = 0.25f
    });
}

static void play_step(int step, float at) {
    if (step % 128 == 0) pick_section();
    int s = step % 16;
    int bar = (step >> 4) % 8;
    const Section *sec = &SECTIONS[gMus.sec];
    int chIdx = sec->bars[bar][s < 8 ? 0 : sec->barLen[bar] - 1];
    const int *ch = CHORDS[chIdx];
    int root = (sec->pedal && bar < 7) ? sec->pedal : CHORDS[sec->bars[bar][0]][0];
    bool change = (s == 0) || (s == 8 && sec->barLen[bar] == 2);
    int span = sec->barLen[bar] == 2 ? 8 : 16;

    const MelNote *mel = NULL;
    int step128 = step % 128;
    for (int i = 0; i < sec->melCount; i++) {
        if (sec->mel[i].step == step128) { mel = &sec->mel[i]; break; }
    }

    if (gMus.mode == MUSIC_TITLE) {
        if (change) play_pad(at, ch, span, 0.026f, 0.9f);
        if (s == 0) {
            synth_spawn(&(VoiceParams){
                .type = WAVE_TRI, .bus = BUS_MUSIC, .delay = at, .f = midi_hz((float)root),
                .hold = (float)15 * mus_sixteenth(), .attack = 0.3f, .release = 0.8f, .vol = 0.28f
            });
        }
        if (gMus.sec == 3) {
            if (s % 6 == 3 && rnd01() < 0.6f) play_bell(at, ch[1 + GetRandomValue(0, 3)] + 12, 0.08f);
            return;
        }
        if (mel) {
            play_bell(at, mel->m, 0.27f);
            if (gMus.vary) {
                int h = harmony_under(mel->m, ch);
                if (h >= 0) play_bell(at + 0.012f, h, 0.12f);
            }
        }
        if (gMus.vary && s % 4 == 2) {
            synth_spawn(&(VoiceParams){
                .type = WAVE_PULSE12, .bus = BUS_MUSIC_PUMP, .delay = at,
                .f = midi_hz((float)(ch[1 + ((s >> 2) % 4)] + 12)),
                .hold = 2.0f * mus_sixteenth(), .attack = 0.005f, .release = 0.08f,
                .vol = 0.035f, .pan = ((s >> 2) & 1) ? 0.35f : -0.35f
            });
        }
    } else if (gMus.mode == MUSIC_CALM) {
        if (change) {
            for (int i = 1; i < 5; i++) {
                synth_spawn(&(VoiceParams){
                    .type = WAVE_SINE, .bus = BUS_MUSIC, .delay = at + (i - 1) * 0.018f,
                    .f = midi_hz((float)ch[i]), .hold = (float)span * mus_sixteenth(),
                    .attack = 0.004f, .release = 0.5f, .vol = hum(0.05f), .pan = -0.1f, .wet = 0.2f
                });
            }
            play_pad(at, ch, span, 0.008f, 0.4f);
        }
        if (s % 4 == 0) {
            int nextBar = (bar + 1) % 8;
            int nextRoot = (sec->pedal && nextBar < 7) ? sec->pedal : CHORDS[sec->bars[nextBar][0]][0];
            int walk[4] = { root, root + 7, root + 12, nextRoot > root ? nextRoot - 1 : nextRoot + 1 };
            play_bass(at, walk[s >> 2], 4, hum(0.24f));
        }
        if (s == 4 || s == 12) {
            synth_spawn(&(VoiceParams){
                .type = WAVE_NOISE_WHITE, .bus = BUS_MUSIC, .delay = at, .filt = FILT_BANDPASS, .filtF = 3500.0f,
                .dur = 0.14f, .attack = 0.03f, .vol = hum(0.05f), .pan = 0.25f
            });
        }
        if (s % 4 == 0) play_hat(at, 0.035f, false);
        else if (s % 2 == 0) play_hat(at, 0.02f, false);
        if (mel && !gMus.vary) {
            synth_spawn(&(VoiceParams){
                .type = WAVE_TRI, .bus = BUS_MUSIC, .delay = at, .f = midi_hz((float)mel->m),
                .hold = (float)mel->l * mus_sixteenth() * 0.92f, .attack = 0.01f, .release = 0.12f,
                .vol = 0.13f, .vib = 0.15f, .vibDelay = 0.18f, .pan = 0.08f, .wet = 0.2f
            });
        }
    } else if (gMus.mode == MUSIC_COMBAT) {
        if (s == 0) {
            float h = mus_heat();
            gMus.layer = h < 0.35f ? 1 : (h < 0.7f ? 2 : 3);
        }
        int L = gMus.layer;
        bool half = (gMus.sec == 2);
        bool last = (bar == 7);
        if (change) play_pad(at, ch, span, 0.014f + 0.007f * L, 0.06f);
        if (s == 0 && bar == 0 && gMus.secN > 1 && L >= 2) {
            synth_spawn(&(VoiceParams){
                .type = WAVE_NOISE_WHITE, .bus = BUS_MUSIC, .delay = at, .filt = FILT_HIGHPASS, .filtF = 4500.0f,
                .dur = 1.8f, .attack = 0.002f, .vol = 0.22f, .pan = -0.3f
            });
        }
        if (half) {
            if (s == 0) play_bass(at, root, 16, 0.42f);
            if (last && s % 2 == 0) play_bass(at, root + 12 * ((s >> 1) & 1), 2, 0.35f);
            if (s == 0 || (L >= 2 && s == 11)) play_kick(at, 0.85f);
            if (s == 8 && !last) play_snare(at, 0.55f);
            if (s % 4 == 2) play_hat(at, 0.07f, false);
            if (last && (s >= 8 || s % 2 == 0)) play_snare(at, 0.12f + s * 0.02f);
            if (mel) play_bell(at, mel->m, 0.2f);
            if (gMus.lowHp && s % 4 == 0) {
                synth_spawn(&(VoiceParams){ .type = WAVE_SINE, .bus = BUS_POST, .delay = at, .f = 62.0f, .f2 = 40.0f, .dur = 0.14f, .attack = 0.002f, .vol = 0.9f });
                synth_spawn(&(VoiceParams){ .type = WAVE_SINE, .bus = BUS_POST, .delay = at + 0.16f, .f = 55.0f, .f2 = 38.0f, .dur = 0.12f, .attack = 0.002f, .vol = 0.63f });
            }
            return;
        }

        static const int gallop[16] = {1,0,1,1,1,0,1,1,1,0,1,1,1,0,1,1};
        if (L < 3 ? (s % 2 == 0) : gallop[s]) {
            int m = root + (s % 4 == 2 ? 12 : 0);
            if (s == 14 && !last) {
                int nb = (sec->pedal && bar + 1 < 7) ? sec->pedal : CHORDS[sec->bars[bar + 1][0]][0];
                if (nb != root) m = nb + (nb > root ? -1 : 1);
            }
            play_bass(at, m, L < 3 ? 2 : 1, hum(0.3f + 0.03f * L));
        }

        bool fill = last && s >= 12 && L >= 2;
        if (fill) {
            const float toms[4] = { 200, 165, 135, 105 };
            float tf = toms[s - 12];
            synth_spawn(&(VoiceParams){ .type = WAVE_SINE, .bus = BUS_MUSIC, .delay = at, .f = tf, .f2 = tf * 0.55f, .dur = 0.22f, .attack = 0.002f, .vol = 0.6f });
        } else {
            if (s == 0 || s == 8 || (L >= 2 && s == 10) || (L >= 3 && s == 3)) play_kick(at, 0.9f);
            if (L >= 2 && (s == 4 || s == 12)) play_snare(at, hum(0.5f));
            if (L >= 3 && (s == 7 || s == 15)) play_snare(at, hum(0.1f));
            bool open = (gMus.sec == 1 && L >= 2 && s % 4 == 2);
            if (L >= 3 || s % 2 == 0) play_hat(at, open ? 0.1f : (s % 4 == 2 ? 0.16f : 0.08f), open);
        }

        if (L == 1 ? (s % 4 == 2) : (L == 2 ? (s % 2 == 0) : true)) {
            int idx = L == 1 ? 4 + ((s >> 2) & 3) : (L == 2 ? (s >> 1) : ARP_UPDOWN[s]);
            int note = ch[1 + (idx % 4)] + (idx >= 4 ? 12 : 0);
            synth_spawn(&(VoiceParams){
                .type = WAVE_PULSE12, .bus = BUS_MUSIC_PUMP, .delay = at, .f = midi_hz((float)note),
                .hold = (float)(L == 3 ? 1 : 2) * mus_sixteenth() * 0.8f, .attack = 0.005f, .release = 0.05f,
                .vol = hum(0.042f), .pan = (s & 2) ? 0.35f : -0.35f, .wet = 0.15f
            });
        }

        if (mel) {
            float mLen = (float)mel->l * mus_sixteenth() * 0.92f;
            synth_spawn(&(VoiceParams){
                .type = (L == 1) ? WAVE_TRI : WAVE_PULSE25, .bus = BUS_MUSIC, .delay = at,
                .f = midi_hz((float)mel->m), .hold = mLen, .attack = 0.01f, .release = 0.12f,
                .vol = (L == 1) ? 0.12f : 0.18f, .vib = mel->l >= 4 ? 0.22f : 0.0f, .vibDelay = 0.18f,
                .pan = 0.08f, .wet = 0.2f
            });
            if (L >= 3) {
                synth_spawn(&(VoiceParams){
                    .type = WAVE_PULSE50, .bus = BUS_MUSIC, .delay = at,
                    .f = midi_hz((float)(mel->m - 12)), .hold = mLen, .attack = 0.01f, .release = 0.1f,
                    .vol = 0.06f, .pan = 0.08f, .wet = 0.2f
                });
            }
            if (L >= 2 && (gMus.vary || gMus.sec == 1)) {
                int h = harmony_under(mel->m, ch);
                if (h >= 0) {
                    synth_spawn(&(VoiceParams){
                        .type = WAVE_PULSE50, .bus = BUS_MUSIC, .delay = at,
                        .f = midi_hz((float)h), .hold = mLen, .attack = 0.01f, .release = 0.12f,
                        .vol = 0.09f, .vib = mel->l >= 4 ? 0.15f : 0.0f, .vibDelay = 0.18f,
                        .pan = -0.22f, .wet = 0.2f
                    });
                }
            }
        }
        if (gMus.lowHp && s % 4 == 0) {
            synth_spawn(&(VoiceParams){ .type = WAVE_SINE, .bus = BUS_POST, .delay = at, .f = 62.0f, .f2 = 40.0f, .dur = 0.14f, .attack = 0.002f, .vol = 0.9f });
            synth_spawn(&(VoiceParams){ .type = WAVE_SINE, .bus = BUS_POST, .delay = at + 0.16f, .f = 55.0f, .f2 = 38.0f, .dur = 0.12f, .attack = 0.002f, .vol = 0.63f });
        }
    }
}

static void music_tick(float dt) {
    if (gMus.mode == MUSIC_DEAD) {
        gMus.reviveT += dt;
        if (gMus.reviveT >= 4.2f) music_set_mode(MUSIC_TITLE);
    }
    synth_set_echo_delay(mus_sixteenth() * 3.0f);

    float stepDur = mus_sixteenth();
    gMus.accum += dt;
    while (gMus.accum >= stepDur) {
        gMus.accum -= stepDur;
        if (gMus.hasPending && gMus.step % 4 == 0) enter_mode(gMus.pending);
        if (gMus.mode != MUSIC_DEAD && gMus.mode != MUSIC_OFF) {
            float swing = (gMus.step % 2) ? (gMus.mode == MUSIC_CALM ? 0.16f : (gMus.mode == MUSIC_TITLE ? 0.06f : 0.0f)) * stepDur : 0.0f;
            play_step(gMus.step, swing);
        }
        gMus.step++;
    }
}

void music_init(void) {
    gMus.mode = MUSIC_OFF;
    gMus.layer = 1;
    synth_set_music_tick(music_tick);
    music_set_mode(MUSIC_TITLE);
}

void music_set_mode(MusicMode mode) {
    MusicMode cur = gMus.hasPending ? gMus.pending : gMus.mode;
    if (mode == cur) return;
    if (mode != MUSIC_DEAD) {
        gMus.pending = mode;
        gMus.hasPending = true;
        return;
    }
    gMus.hasPending = false;
    gMus.mode = MUSIC_DEAD;
    gMus.reviveT = 0.0f;
    synth_set_music_lp(120.0f);
    synth_set_music_duck_target(0.0f, 0.3f);
}

void music_update_state(int wave, int alive, bool lowHp, float dt) {
    (void)dt;
    gMus.wave = wave;
    gMus.alive = alive;
    if (lowHp != gMus.lowHp) {
        gMus.lowHp = lowHp;
        apply_filter();
    }
}
