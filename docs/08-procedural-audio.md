# Chapter 8: Procedural Audio & Adaptive Music

In the original Three.js game, every sound effect and every musical note is synthesized in real time using the Web Audio API — there are **zero `.wav`, `.mp3`, or `.ogg` asset files**. In Raylib, we recreate that exact same philosophy using `AudioStream` and a custom C11 software synthesizer running on Raylib's audio thread.

---

## 1. Raylib's `AudioStream` & Thread Safety

Raylib gives us direct sample-level control via `LoadAudioStream`:

```c
InitAudioDevice();
SetAudioStreamBufferSizeDefault(4096);
AudioStream stream = LoadAudioStream(SAMPLE_RATE, 16, 2); // 44100 Hz, 16-bit, stereo
SetAudioStreamCallback(stream, audio_callback);
PlayAudioStream(stream);
```

Because `audio_callback(void *bufferData, unsigned int frames)` runs on miniaudio's background audio thread while gameplay (`sfx_shot`, `music_update_state`, etc.) runs on the main thread, shared synth state (active voices, weather levels, ducking envelopes, music scheduler state) is guarded by a recursive mutex (`PTHREAD_MUTEX_RECURSIVE` initialized in `synth_init()`):

```c
pthread_mutex_lock(&gMutex);
// Render `frames` stereo samples into `buffer` (short interleaved L/R)
// Safe for gMusicTick() to re-enter synth_spawn() / synth_set_music_*()
pthread_mutex_unlock(&gMutex);
```

---

## 2. Band-Limited Wavetables & LFSR Noise

Naive square or sawtooth waves (`x < duty ? 1 : -1`) alias harshly at high pitches. In `src/audio/synth.c`, `synth_init()` precomputes band-limited wavetables via additive Fourier synthesis:

- `WAVE_TRI` — odd harmonics \(1, 3, 5, \dots\) with alternating signs and \(1/n^2\) roll-off.
- `WAVE_PULSE12`, `WAVE_PULSE25`, `WAVE_PULSE50` — duty-cycle pulse waves built from \(\frac{2}{n\pi}\sin(n \pi d) \cos(n \theta)\).
- `WAVE_SAW` — all harmonics \(1..N\) with \(1/n\) roll-off.

For retro percussion, wind, and thunder, we also generate **NES-style 15-bit LFSR noise**:
- `WAVE_NOISE_LONG` — 32,767-sample white-ish LFSR sequence.
- `WAVE_NOISE_METAL` — 93-sample looped LFSR sequence for metallic clanks and hi-hats.
- `WAVE_NOISE_WHITE` — unpitched white noise for gunshots, rain, and wind.

---

## 3. Voices, Envelopes, Filters & Formant Growls

Each sound is triggered via `synth_play(VoiceParams p)` (`src/audio/synth.h`):

- **Pitch Sweeps & Vibrato**: Supports exponential frequency sweeps (`f` → `f2`), `detune` in cents, and delayed vibrato (`vib`, `vibDelay`).
- **Envelopes**: Supports both percussive exponential decay (`dur`) and gated ADSR-style sustain (`hold`, `attack`, `release`).
- **State-Variable Filter (SVF)**: Per-voice `FILT_LOWPASS`, `FILT_BANDPASS`, or `FILT_HIGHPASS` with sweeping cutoff (`filtF` → `filtF2`) and resonance (`filtQ`).
- **Dual-Formant Zombie Growl (`WAVE_GROWL`)**: A sawtooth oscillator routed through two parallel bandpass filters (`formant` and `formant * 2.1`) to create guttural vocal timbre.

---

## 4. Master Bus: Wind, Dotted-8th Echo, Reverb & Limiter

Inside the audio callback, all active voices mix into four buses (`BUS_SFX`, `BUS_POST`, `BUS_MUSIC`, `BUS_MUSIC_PUMP`) before passing through:

1. **Ambient Rain & Wind**: Filtered noise driven smoothly by `synth_set_weather(level, gust)`.
2. **Dotted-Eighth Stereo Echo**: A feedback delay line at `0.375`s (synced to 120 BPM) for atmospheric leads.
3. **4-Comb Schroeder Reverb**: Parallel comb filters (`wet` send per voice) giving the forest and cabin depth.
4. **Sidechain Ducking & Muffle**: Heavy impacts (`synth_duck`) and pause/death states (`synth_muffle`) smoothly attenuate and low-pass the mix.
5. **Soft Limiter**: `tanhf(sample)` clamps peaks cleanly without harsh digital clipping.

---

## 5. Procedural Sound Effects (`src/audio/sfx.c`)

Every gameplay event builds layered `VoiceParams` calls with **distance attenuation and stereo panning** relative to the player (`pos.x - gPlayer.position.x`):

| Event | Synthesis Recipe |
|---|---|
| `sfx_shot(hot, spread)` | White-noise crack + low-end triangle punch + metallic bolt click; hotter & wider when `hot` or `spread > 0` |
| `sfx_hit` / `sfx_zdeath` | Bandpassed flesh thud + bone crack; `sfx_zdeath` adds a downward formant groan |
| `sfx_swipe` | Fast bandpass noise whoosh |
| `sfx_bash` / `sfx_door_break` | Low resonant wood thump + splintering noise burst |
| `sfx_crack` / `sfx_shatter` | High-Q metallic/glass chime partials + crisp white-noise burst |
| `sfx_step` | Short low-pass noise thud on each footfall in `player_animate()` |
| `sfx_reload_out` / `sfx_reload_in` | Crisp mechanical magazine release & lock clicks |
| `sfx_thunder(nearVal)` | Delayed low-passed LFSR rumble scaled by lightning distance |

---

## 6. Adaptive Chiptune Sequencer (`src/audio/music.c`)

Rather than looping a static track, `music_update_state(wave, alive, lowHp, dt)` advances a step sequencer inside the audio/main loop across four modes:

- `MUSIC_TITLE` — moody sparse arpeggio over the dark cabin title screen.
- `MUSIC_CALM` — reflective D-minor / F-major breather between waves.
- `MUSIC_COMBAT` — driving bassline, snare/kick percussion, and rotating melodic sections (`THEME`, `SOAR`, `OMEN`, `HAVEN`) across an 18-chord progression. Intensity scales with `wave`, `alive`, and `lowHp`.
- `MUSIC_DEAD` — slow descending minor cadence on game over.

---

**Next:** [Chapter 9: Creating the UI & Full Game Loop](09-creating-the-ui.md)


---

## Hands-On Example

- [`examples/ex08_procedural_synth.c`](../examples/ex08_procedural_synth.c) — `AudioStream` callback with `PTHREAD_MUTEX_RECURSIVE`, 15-bit LFSR noise, SVF filter, and interactive SFX/chord keys `1–4` (`-DBUILD_LESSON_EXAMPLE=ex08_procedural_synth`).
