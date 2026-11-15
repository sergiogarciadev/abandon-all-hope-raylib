#ifndef SYNTH_H
#define SYNTH_H

#include <stdbool.h>

#define SAMPLE_RATE 44100

typedef enum {
    WAVE_SINE = 0,
    WAVE_TRI,
    WAVE_PULSE12,
    WAVE_PULSE25,
    WAVE_PULSE50,
    WAVE_SAW,
    WAVE_NOISE_LONG,
    WAVE_NOISE_METAL,
    WAVE_NOISE_WHITE,
    WAVE_GROWL
} VoiceType;

typedef enum {
    FILT_NONE = 0,
    FILT_LOWPASS,
    FILT_BANDPASS,
    FILT_HIGHPASS
} FilterType;

typedef enum {
    BUS_SFX = 0,
    BUS_POST,
    BUS_MUSIC,
    BUS_MUSIC_PUMP
} AudioBus;

typedef struct {
    VoiceType type;
    AudioBus bus;
    float delay;    // seconds before start
    float f;        // start freq (Hz)
    float f2;       // end freq (Hz), 0 = no sweep
    float dur;      // percussive duration (s)
    float hold;     // > 0 for gated sustained note
    float attack;   // attack time (s)
    float release;  // release time (s) for hold notes
    float vol;      // peak amplitude
    float pan;      // -1..1
    float wet;      // reverb send 0..1
    float vib;      // vibrato depth in semitones
    float vibDelay; // seconds before vibrato ramps in
    float detune;   // cents
    float rate;     // noise playback rate (1.0 default)
    FilterType filt;
    float filtF;
    float filtF2;   // 0 = no sweep
    float filtQ;
    float formant;  // for WAVE_GROWL
} VoiceParams;

void synth_init(void);
void synth_cleanup(void);
void synth_spawn(const VoiceParams *p);

void synth_set_volumes(int masterLvl, int musicLvl, bool muted);
void synth_set_paused(bool paused);
void synth_set_weather(float level, float gust);
void synth_duck_music(float depth, float dur);
void synth_muffle(float dur, float freq);
void synth_set_music_lp(float freq);
void synth_set_music_duck_target(float target, float tau);
void synth_trigger_pump(void);
void synth_set_echo_delay(float sec);

typedef void (*MusicTickFn)(float dt);
void synth_set_music_tick(MusicTickFn fn);

#endif // SYNTH_H
