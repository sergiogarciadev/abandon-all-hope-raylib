#ifndef WEATHER_H
#define WEATHER_H

#include <stdbool.h>

#define BOLT_LEVEL 0.7f

typedef struct {
    float level;
    float target;
    float t;
    float drop;
    float gust;
    float flashT;
    float flash;
    float nextBolt;
    float thunderIn;
    float thunderNear;
    int splashI;
    float splashDue;
    float soundT;
} WeatherState;

extern WeatherState gWeather;

void weather_init(void);
void weather_set(float level);
float storm_for_wave(int wave);
void weather_strike_lightning(void);
void weather_update(float dt, bool running);
void weather_draw(void);

#endif // WEATHER_H
