#ifndef SFX_H
#define SFX_H

#include "raylib.h"
#include <stdbool.h>

void sfx_shot(bool hot, int spread);
void sfx_hit(Vector3 pos);
void sfx_zdeath(Vector3 pos);
void sfx_groan(Vector3 pos);
void sfx_swipe(Vector3 pos);
void sfx_bash(Vector3 pos, float scale);
void sfx_crack(Vector3 pos);
void sfx_shatter(Vector3 pos);
void sfx_door_break(Vector3 pos);
void sfx_step(void);
void sfx_reload_out(void);
void sfx_reload_in(void);
void sfx_hurt(void);
void sfx_pickup(int kind);
void sfx_wave_start(int n);
void sfx_wave_clear(void);
void sfx_deal(int count);
void sfx_hover(int idx);
void sfx_upgrade(int catIdx);
void sfx_death(void);
void sfx_thunder(float nearVal);
void sfx_ui(const char *kind);

#endif // SFX_H
