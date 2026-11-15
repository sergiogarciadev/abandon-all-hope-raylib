#ifndef TITLE_H
#define TITLE_H

#include "raylib.h"
#include <stdbool.h>

#define TITLE_ZOMBIES 4
#define SWOOP_TIME    1.8f
#define TITLE_FOV     42.0f
#define GAME_FOV      55.0f

typedef enum {
    SCREEN_TITLE = 0,
    SCREEN_HOWTO,
    SCREEN_SETTINGS,
    SCREEN_PLAYING,
    SCREEN_PAUSED,
    SCREEN_DEAD
} ScreenMode;

typedef struct {
    void (*onPlay)(void);
    void (*onResume)(void);
    void (*onRestart)(void);
    void (*onQuitToTitle)(void);
    void (*onSettingsChanged)(void);
} TitleCallbacks;

extern ScreenMode gScreen;
extern float gSwoopT;

void title_init(TitleCallbacks cbs);
void title_set_screen(ScreenMode mode);
void title_start_swoop(void);
void title_set_death_info(int wave, int kills, bool isNewBest);
void title_update(float dt, Camera3D *cam, Vector3 playerPos);
void title_draw(void);
bool title_should_quit(void);

#endif // TITLE_H
