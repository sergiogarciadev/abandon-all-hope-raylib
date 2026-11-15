#include "synth.h"
#include "core/math_util.h"
#include "raylib.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#if defined(PLATFORM_WEB)
#define AUDIO_LOCK()   ((void)0)
#define AUDIO_UNLOCK() ((void)0)
#else
#include <pthread.h>
#endif

#define MAX_VOICES 96
#define TABLE_SIZE 512
#define ECHO_MAX_SAMPLES 44100
#define COMB_COUNT 4

typedef struct {
    bool active;
    VoiceParams p;
    uint32_t delaySamples;
    uint32_t ageSamples;
    uint32_t totalSamples;
    uint32_t attackSamples;
    uint32_t holdSamples;

    float phase;
    float phase2;
    float vibPhase;
    float noisePos;
    float env;

    // Biquad / SVF state
    float lp1, bp1;
    float lp2, bp2;
} Voice;

static AudioStream gStream;
static bool gAudioReady = false;
#if !defined(PLATFORM_WEB)
static pthread_mutex_t gMutex = PTHREAD_MUTEX_INITIALIZER;
#define AUDIO_LOCK()   pthread_mutex_lock(&gMutex)
#define AUDIO_UNLOCK() pthread_mutex_unlock(&gMutex)
#endif

static float gWaveTables[5][TABLE_SIZE]; // sine, tri, pulse12, pulse25, pulse50
static float *gNoiseLong = NULL;
static float *gNoiseMetal = NULL;
static float *gNoiseWhite = NULL;
static int gNoiseLongLen = SAMPLE_RATE * 2;
static int gNoiseMetalLen = 93 * 64;
static int gNoiseWhiteLen = SAMPLE_RATE * 2;

static Voice gVoices[MAX_VOICES];
static MusicTickFn gMusicTick = NULL;

// Mix state
static float gMasterTarget = 0.8f, gMasterCur = 0.8f;
static float gMusicTarget = 1.0f, gMusicCur = 1.0f;
static float gPauseTarget = 1.0f, gPauseCur = 1.0f;
static bool gMuted = false;

static float gDuckCur = 1.0f, gDuckTarget = 1.0f, gDuckHoldT = 0.0f, gDuckTau = 0.3f;
static float gPumpCur = 1.0f;
static float gMusicLpFreq = 18000.0f, gMusicLpTarget = 18000.0f;
static float gMuffleFreq = 20000.0f, gMuffleHoldT = 0.0f, gMuffleTau = 0.35f;

// Filter states for buses
static float gMusicLpL = 0, gMusicLpR = 0;
static float gMuffleLpL = 0, gMuffleLpR = 0;

// Echo delay buffer
static float gEchoL[ECHO_MAX_SAMPLES];
static float gEchoR[ECHO_MAX_SAMPLES];
static int gEchoIdx = 0;
static float gEchoDelaySec = 0.34f;
static float gEchoDampL = 0, gEchoDampR = 0;

// Simple 4-comb reverb
static const int COMB_LENS[COMB_COUNT] = { 1557, 1617, 1491, 1422 };
static float gCombBufL[COMB_COUNT][1700];
static float gCombBufR[COMB_COUNT][1700];
static int gCombIdx[COMB_COUNT];
static float gCombLpL[COMB_COUNT], gCombLpR[COMB_COUNT];

// Ambient wind
static float gWindPhase = 0, gWindLfo1 = 0, gWindLfo2 = 0;
static float gWindBp = 0, gWindLp = 0;
static float gWindGain = 0.0f, gWindTarget = 0.06f;

static void build_tables(void) {
    // Band-limited additive synthesis for chip waves (32 harmonics)
    for (int i = 0; i < TABLE_SIZE; i++) {
        float ph = (float)i / (float)TABLE_SIZE;
        gWaveTables[WAVE_SINE][i] = sinf(2.0f * PI * ph);

        // 32-step 4-bit NES triangle
        int step = (int)(ph * 32.0f);
        gWaveTables[WAVE_TRI][i] = (float)(step < 16 ? 15 - step : step - 16) / 7.5f - 1.0f;

        // Band-limited pulses via Fourier series
        const float duties[3] = { 0.125f, 0.25f, 0.5f };
        for (int d = 0; d < 3; d++) {
            float sum = 0.0f;
            for (int h = 1; h <= 24; h++) {
                float coeff = (2.0f / ((float)h * PI)) * sinf((float)h * PI * duties[d]);
                sum += coeff * cosf(2.0f * PI * (float)h * ph);
            }
            gWaveTables[WAVE_PULSE12 + d][i] = sum * 1.4f;
        }
    }

    // 15-bit LFSR noise
    gNoiseLong = malloc((size_t)gNoiseLongLen * sizeof(float));
    uint32_t r = 1;
    for (int i = 0; i < gNoiseLongLen; i++) {
        uint32_t fb = (r & 1u) ^ ((r >> 1u) & 1u);
        r = (r >> 1u) | (fb << 14u);
        gNoiseLong[i] = (r & 1u) ? 0.8f : -0.8f;
    }

    gNoiseMetal = malloc((size_t)gNoiseMetalLen * sizeof(float));
    r = 1;
    for (int i = 0; i < gNoiseMetalLen; i++) {
        uint32_t fb = (r & 1u) ^ ((r >> 6u) & 1u);
        r = (r >> 1u) | (fb << 14u);
        gNoiseMetal[i] = (r & 1u) ? 0.8f : -0.8f;
    }

    gNoiseWhite = malloc((size_t)gNoiseWhiteLen * sizeof(float));
    SeededRng wrng = rng_seed(1337);
    for (int i = 0; i < gNoiseWhiteLen; i++) {
        gNoiseWhite[i] = rng_next(&wrng) * 2.0f - 1.0f;
    }
}

static inline float sample_table(int tbl, float phase) {
    float p = phase - floorf(phase);
    float idx = p * (float)TABLE_SIZE;
    int i0 = (int)idx % TABLE_SIZE;
    int i1 = (i0 + 1) % TABLE_SIZE;
    float frac = idx - floorf(idx);
    return gWaveTables[tbl][i0] + (gWaveTables[tbl][i1] - gWaveTables[tbl][i0]) * frac;
}

static inline float svf_filter(float in, float cutoff, float q, FilterType type, float *lp, float *bp) {
    float f = 2.0f * sinf(PI * clampf(cutoff, 20.0f, 19000.0f) / (float)SAMPLE_RATE);
    float damp = 1.0f / fmaxf(q, 0.5f);
    *lp += f * (*bp);
    float hp = in - *lp - damp * (*bp);
    *bp += f * hp;
    if (type == FILT_LOWPASS) return *lp;
    if (type == FILT_HIGHPASS) return hp;
    return *bp;
}

static void audio_callback(void *buffer, unsigned int frames) {
    short *out = (short *)buffer;
    const float dt = 1.0f / (float)SAMPLE_RATE;

    AUDIO_LOCK();

    if (gMusicTick) {
        gMusicTick((float)frames * dt);
    }

    for (unsigned int f = 0; f < frames; f++) {
        // Smooth mix parameters
        gMasterCur += (gMasterTarget - gMasterCur) * 0.002f;
        gMusicCur  += (gMusicTarget - gMusicCur) * 0.002f;
        gPauseCur  += (gPauseTarget - gPauseCur) * 0.001f;
        gPumpCur   += (1.0f - gPumpCur) * (dt / 0.09f);
        gMusicLpFreq += (gMusicLpTarget - gMusicLpFreq) * (dt / 0.25f);

        if (gDuckHoldT > 0.0f) {
            gDuckHoldT -= dt;
        } else {
            gDuckCur += (gDuckTarget - gDuckCur) * (dt / fmaxf(gDuckTau, 0.02f));
        }
        if (gMuffleHoldT > 0.0f) {
            gMuffleHoldT -= dt;
        } else {
            gMuffleFreq += (20000.0f - gMuffleFreq) * (dt / fmaxf(gMuffleTau, 0.02f));
        }
        gWindGain += (gWindTarget - gWindGain) * (dt / 0.4f);

        float sfxL = 0, sfxR = 0;
        float postL = 0, postR = 0;
        float musL = 0, musR = 0;
        float verbInL = 0, verbInR = 0;

        // Ambient wind
        gWindPhase += 1.0f;
        int wi = ((int)gWindPhase) % gNoiseWhiteLen;
        gWindLfo1 += 0.07f * dt;
        gWindLfo2 += 0.13f * dt;
        float wFreq = 420.0f + 220.0f * sinf(2.0f * PI * gWindLfo1);
        float wMod = gWindGain + 0.025f * sinf(2.0f * PI * gWindLfo2);
        float wSig = svf_filter(gNoiseWhite[wi], wFreq, 1.1f, FILT_BANDPASS, &gWindLp, &gWindBp) * fmaxf(0.0f, wMod);
        sfxL += wSig;
        sfxR += wSig;

        for (int i = 0; i < MAX_VOICES; i++) {
            Voice *v = &gVoices[i];
            if (!v->active) continue;
            if (v->delaySamples > 0) {
                v->delaySamples--;
                continue;
            }

            float prog = (float)v->ageSamples / (float)v->totalSamples;
            if (v->ageSamples >= v->totalSamples) {
                v->active = false;
                continue;
            }

            // Envelope
            float env = 1.0f;
            if (v->ageSamples < v->attackSamples) {
                env = (float)v->ageSamples / (float)v->attackSamples;
            } else if (v->p.hold > 0.0f) {
                if (v->ageSamples < v->holdSamples) {
                    float t = (float)(v->ageSamples - v->attackSamples) * dt;
                    env = 0.6f + 0.4f * expf(-t / fmaxf(v->p.hold * 0.4f, 0.05f));
                } else {
                    float rt = (float)(v->ageSamples - v->holdSamples) / (float)(v->totalSamples - v->holdSamples);
                    env = 0.6f * (1.0f - rt);
                }
            } else {
                float t = (float)(v->ageSamples - v->attackSamples) / (float)(v->totalSamples - v->attackSamples);
                env = powf(1.0f - t, 2.8f);
            }

            // Frequency
            float freq = v->p.f;
            if (v->p.f2 > 0.0f) {
                float k = clampf(prog, 0.0f, 1.0f);
                freq = v->p.f * powf(v->p.f2 / v->p.f, k);
            }
            if (v->p.detune != 0.0f) {
                freq *= powf(2.0f, v->p.detune / 1200.0f);
            }
            if (v->p.vib > 0.0f) {
                float ageT = (float)v->ageSamples * dt;
                float vibDepth = clampf((ageT - v->p.vibDelay) / 0.25f, 0.0f, 1.0f);
                v->vibPhase += 5.3f * dt;
                freq *= 1.0f + vibDepth * (powf(2.0f, v->p.vib / 12.0f) - 1.0f) * sinf(2.0f * PI * v->vibPhase);
            }

            // Sample generation
            float sig = 0.0f;
            if (v->p.type <= WAVE_PULSE50) {
                v->phase += freq * dt;
                sig = sample_table((int)v->p.type, v->phase);
            } else if (v->p.type == WAVE_SAW) {
                v->phase += freq * dt;
                float p = v->phase - floorf(v->phase);
                sig = 2.0f * p - 1.0f;
            } else if (v->p.type == WAVE_GROWL) {
                v->vibPhase += 6.5f * dt;
                float fm = freq * (1.0f + 0.06f * sinf(2.0f * PI * v->vibPhase));
                v->phase += fm * dt;
                v->phase2 += fm * 1.0052f * dt;
                float s1 = 2.0f * (v->phase - floorf(v->phase)) - 1.0f;
                float s2 = 2.0f * (v->phase2 - floorf(v->phase2)) - 1.0f;
                float raw = (s1 + s2) * 0.5f;
                float f1 = svf_filter(raw, v->p.formant, 2.5f, FILT_BANDPASS, &v->lp1, &v->bp1);
                float f2 = svf_filter(raw, v->p.formant * 1.8f, 4.0f, FILT_BANDPASS, &v->lp2, &v->bp2);
                sig = (f1 + f2) * 0.7f;
            } else {
                const float *nbuf = (v->p.type == WAVE_NOISE_METAL) ? gNoiseMetal
                                  : (v->p.type == WAVE_NOISE_WHITE) ? gNoiseWhite : gNoiseLong;
                int nlen = (v->p.type == WAVE_NOISE_METAL) ? gNoiseMetalLen
                         : (v->p.type == WAVE_NOISE_WHITE) ? gNoiseWhiteLen : gNoiseLongLen;
                v->noisePos += (v->p.rate > 0.0f ? v->p.rate : 1.0f);
                int idx = ((int)v->noisePos) % nlen;
                sig = nbuf[idx];
            }

            if (v->p.filt != FILT_NONE && v->p.type != WAVE_GROWL) {
                float fc = v->p.filtF;
                if (v->p.filtF2 > 0.0f) {
                    fc = v->p.filtF * powf(v->p.filtF2 / v->p.filtF, clampf(prog, 0.0f, 1.0f));
                }
                sig = svf_filter(sig, fc, v->p.filtQ, v->p.filt, &v->lp1, &v->bp1);
            }

            sig *= env * v->p.vol;

            float pan = clampf(v->p.pan, -0.9f, 0.9f);
            float gL = cosf((pan + 1.0f) * PI * 0.25f);
            float gR = sinf((pan + 1.0f) * PI * 0.25f);
            float sL = sig * gL;
            float sR = sig * gR;

            if (v->p.bus == BUS_POST) {
                postL += sL; postR += sR;
            } else if (v->p.bus == BUS_MUSIC) {
                musL += sL; musR += sR;
            } else if (v->p.bus == BUS_MUSIC_PUMP) {
                musL += sL * gPumpCur; musR += sR * gPumpCur;
            } else {
                sfxL += sL; sfxR += sR;
            }

            if (v->p.wet > 0.0f) {
                verbInL += sL * v->p.wet;
                verbInR += sR * v->p.wet;
            }

            v->ageSamples++;
        }

        // Music echo (dotted-8th)
        int dSamples = (int)clampf(gEchoDelaySec * (float)SAMPLE_RATE, 100.0f, (float)(ECHO_MAX_SAMPLES - 1));
        int readIdx = (gEchoIdx - dSamples + ECHO_MAX_SAMPLES) % ECHO_MAX_SAMPLES;
        float eL = gEchoL[readIdx];
        float eR = gEchoR[readIdx];
        gEchoDampL += (eL - gEchoDampL) * 0.3f;
        gEchoDampR += (eR - gEchoDampR) * 0.3f;
        gEchoL[gEchoIdx] = musL * 0.35f + gEchoDampR * 0.36f;
        gEchoR[gEchoIdx] = musR * 0.35f + gEchoDampL * 0.36f;
        gEchoIdx = (gEchoIdx + 1) % ECHO_MAX_SAMPLES;
        musL += gEchoDampL * 0.45f;
        musR += gEchoDampR * 0.45f;

        // Music lowpass + duck + volume
        float mlpAlpha = 1.0f - expf(-2.0f * PI * clampf(gMusicLpFreq, 60.0f, 19000.0f) * dt);
        gMusicLpL += (musL - gMusicLpL) * mlpAlpha;
        gMusicLpR += (musR - gMusicLpR) * mlpAlpha;
        float mGain = gMusicCur * gDuckCur * 0.55f;
        float mOutL = gMusicLpL * mGain;
        float mOutR = gMusicLpR * mGain;

        // 4-comb dark reverb
        float vOutL = 0, vOutR = 0;
        for (int c = 0; c < COMB_COUNT; c++) {
            int ci = gCombIdx[c];
            float dL = gCombBufL[c][ci];
            float dR = gCombBufR[c][ci];
            gCombLpL[c] += (dL - gCombLpL[c]) * 0.35f;
            gCombLpR[c] += (dR - gCombLpR[c]) * 0.35f;
            gCombBufL[c][ci] = verbInL + gCombLpL[c] * 0.72f;
            gCombBufR[c][ci] = verbInR + gCombLpR[c] * 0.72f;
            gCombIdx[c] = (ci + 1) % COMB_LENS[c];
            vOutL += dL;
            vOutR += dR;
        }
        vOutL *= 0.14f;
        vOutR *= 0.14f;

        // Master + Muffle
        float preL = (sfxL + mOutL + vOutL) * 0.8f;
        float preR = (sfxR + mOutR + vOutR) * 0.8f;
        float mufAlpha = 1.0f - expf(-2.0f * PI * clampf(gMuffleFreq, 100.0f, 20000.0f) * dt);
        gMuffleLpL += (preL - gMuffleLpL) * mufAlpha;
        gMuffleLpR += (preR - gMuffleLpR) * mufAlpha;

        float totalGain = (gMuted ? 0.0f : gMasterCur) * gPauseCur;
        float finalL = tanhf((gMuffleLpL + postL) * totalGain);
        float finalR = tanhf((gMuffleLpR + postR) * totalGain);

        out[f * 2 + 0] = (short)(clampf(finalL, -1.0f, 1.0f) * 32767.0f);
        out[f * 2 + 1] = (short)(clampf(finalR, -1.0f, 1.0f) * 32767.0f);
    }

    AUDIO_UNLOCK();
}

void synth_init(void) {
#if !defined(PLATFORM_WEB)
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&gMutex, &attr);
    pthread_mutexattr_destroy(&attr);
#endif

    build_tables();
    memset(gVoices, 0, sizeof(gVoices));
    InitAudioDevice();
    if (!IsAudioDeviceReady()) return;

    SetAudioStreamBufferSizeDefault(1024);
    gStream = LoadAudioStream(SAMPLE_RATE, 16, 2);
    SetAudioStreamCallback(gStream, audio_callback);
    PlayAudioStream(gStream);
    gAudioReady = true;
}

void synth_cleanup(void) {
    if (gAudioReady) {
        StopAudioStream(gStream);
        UnloadAudioStream(gStream);
        CloseAudioDevice();
        gAudioReady = false;
    }
    free(gNoiseLong); gNoiseLong = NULL;
    free(gNoiseMetal); gNoiseMetal = NULL;
    free(gNoiseWhite); gNoiseWhite = NULL;
#if !defined(PLATFORM_WEB)
    pthread_mutex_destroy(&gMutex);
#endif
}

void synth_spawn(const VoiceParams *p) {
    if (!gAudioReady) return;
    AUDIO_LOCK();
    int slot = -1;
    for (int i = 0; i < MAX_VOICES; i++) {
        if (!gVoices[i].active) { slot = i; break; }
    }
    if (slot < 0) slot = GetRandomValue(0, MAX_VOICES - 1);

    Voice *v = &gVoices[slot];
    memset(v, 0, sizeof(*v));
    v->active = true;
    v->p = *p;
    if (v->p.filtQ <= 0.0f) v->p.filtQ = 0.7f;
    v->delaySamples = (uint32_t)fmaxf(0.0f, p->delay * (float)SAMPLE_RATE);
    v->attackSamples = (uint32_t)fmaxf(1.0f, p->attack * (float)SAMPLE_RATE);

    if (p->hold > 0.0f) {
        float rel = p->release > 0.0f ? p->release : 0.06f;
        v->holdSamples = (uint32_t)fmaxf((float)v->attackSamples + 1.0f, p->hold * (float)SAMPLE_RATE);
        v->totalSamples = v->holdSamples + (uint32_t)(rel * (float)SAMPLE_RATE);
    } else {
        float d = p->dur > 0.01f ? p->dur : 0.1f;
        v->totalSamples = (uint32_t)(d * (float)SAMPLE_RATE) + 64;
    }

    if (p->type == WAVE_NOISE_LONG || p->type == WAVE_NOISE_METAL || p->type == WAVE_NOISE_WHITE) {
        v->noisePos = rndf(0.0f, 4096.0f);
    }
    AUDIO_UNLOCK();
}

void synth_set_volumes(int masterLvl, int musicLvl, bool muted) {
    AUDIO_LOCK();
    float m = (float)masterLvl / 8.0f;
    float mu = (float)musicLvl / 8.0f;
    gMasterTarget = m * m;
    gMusicTarget = mu * mu;
    gMuted = muted;
    AUDIO_UNLOCK();
}

void synth_set_paused(bool paused) {
    AUDIO_LOCK();
    gPauseTarget = paused ? 0.0f : 1.0f;
    AUDIO_UNLOCK();
}

void synth_set_weather(float level, float gust) {
    AUDIO_LOCK();
    gWindTarget = 0.06f + 0.2f * level * level * gust;
    AUDIO_UNLOCK();
}

void synth_duck_music(float depth, float dur) {
    AUDIO_LOCK();
    gDuckCur = depth;
    gDuckTarget = 1.0f;
    gDuckHoldT = dur;
    gDuckTau = fmaxf(0.05f, dur * 0.4f);
    AUDIO_UNLOCK();
}

void synth_muffle(float dur, float freq) {
    AUDIO_LOCK();
    gMuffleFreq = freq;
    gMuffleHoldT = dur * 0.4f;
    gMuffleTau = fmaxf(0.05f, dur * 0.35f);
    AUDIO_UNLOCK();
}

void synth_set_music_lp(float freq) {
    AUDIO_LOCK();
    gMusicLpTarget = freq;
    AUDIO_UNLOCK();
}

void synth_set_music_duck_target(float target, float tau) {
    AUDIO_LOCK();
    gDuckTarget = target;
    gDuckHoldT = 0.0f;
    gDuckTau = tau;
    AUDIO_UNLOCK();
}

void synth_trigger_pump(void) {
    gPumpCur = 0.3f;
}

void synth_set_echo_delay(float sec) {
    gEchoDelaySec = sec;
}

void synth_set_music_tick(MusicTickFn fn) {
    AUDIO_LOCK();
    gMusicTick = fn;
    AUDIO_UNLOCK();
}
