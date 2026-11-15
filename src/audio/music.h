#ifndef MUSIC_H
#define MUSIC_H

#include <stdbool.h>

typedef enum {
    MUSIC_OFF = 0,
    MUSIC_TITLE,
    MUSIC_CALM,
    MUSIC_COMBAT,
    MUSIC_DEAD
} MusicMode;

void music_init(void);
void music_set_mode(MusicMode mode);
void music_update_state(int wave, int alive, bool lowHp, float dt);

#endif // MUSIC_H
