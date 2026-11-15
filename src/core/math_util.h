#ifndef MATH_UTIL_H
#define MATH_UTIL_H

#include "raylib.h"
#include "raymath.h"
#include <math.h>
#include <stdint.h>

#ifndef PI
#define PI 3.14159265358979323846f
#endif

static inline float midi_hz(float m) {
    return 440.0f * powf(2.0f, (m - 69.0f) / 12.0f);
}

static inline float rndf(float a, float b) {
    return a + ((float)GetRandomValue(0, 1000000) / 1000000.0f) * (b - a);
}

static inline float rnd01(void) {
    return (float)GetRandomValue(0, 1000000) / 1000000.0f;
}

typedef struct {
    uint32_t state;
} SeededRng;

static inline SeededRng rng_seed(uint32_t s) {
    return (SeededRng){ .state = s };
}

static inline float rng_next(SeededRng *r) {
    r->state = r->state * 1664525u + 1013904223u;
    return (float)r->state / 4294967296.0f;
}

static inline float wrap_angle(float a) {
    return a - roundf(a / (2.0f * PI)) * (2.0f * PI);
}

static inline float dampf(float a, float b, float k, float dt) {
    return a + (b - a) * (1.0f - expf(-k * dt));
}

static inline float ease_smooth(float t) {
    return t * t * (3.0f - 2.0f * t);
}

static inline float clampf(float v, float lo, float hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

static inline float lerpf(float a, float b, float t) {
    return a + (b - a) * t;
}

static inline Color hex_color(uint32_t hex, uint8_t alpha) {
    return (Color){
        (uint8_t)((hex >> 16) & 0xFF),
        (uint8_t)((hex >> 8) & 0xFF),
        (uint8_t)(hex & 0xFF),
        alpha
    };
}

static inline Color scale_color(Color c, float k) {
    return (Color){
        (uint8_t)clampf((float)c.r * k, 0.0f, 255.0f),
        (uint8_t)clampf((float)c.g * k, 0.0f, 255.0f),
        (uint8_t)clampf((float)c.b * k, 0.0f, 255.0f),
        c.a
    };
}

#endif // MATH_UTIL_H
