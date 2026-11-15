#ifndef WEAPON_H
#define WEAPON_H

#include "raylib.h"
#include <stdbool.h>

#define MAX_BULLETS 128
#define MAX_PIERCE_HITS 8

typedef struct {
    bool active;
    Vector3 pos;
    Vector3 dir;
    float life;
    int pierce;
    bool hot;
    int hitIds[MAX_PIERCE_HITS];
    int hitCount;
} Bullet;

extern Bullet gBullets[MAX_BULLETS];

void weapon_clear(void);
void weapon_start_reload(void);
void weapon_finish_reload(void);
void weapon_fire(Vector3 aimPoint);
void weapon_update_bullets(float dt);
void weapon_draw_bullets(void);

#endif // WEAPON_H
