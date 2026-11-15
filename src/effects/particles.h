#ifndef PARTICLES_H
#define PARTICLES_H

#include "raylib.h"
#include <stdbool.h>

#define MAX_BLOOD_PARTICLES 256
#define MAX_DEBRIS 256

typedef enum {
    DEBRIS_CASING = 0,
    DEBRIS_MAG,
    DEBRIS_WOOD,
    DEBRIS_GLASS,
    DEBRIS_SMOKE
} DebrisKind;

void effects_init(void);
void effects_clear(void);
void effects_cleanup(void);

void spawn_blood(Vector3 pos);
void spawn_debris(DebrisKind kind, Vector3 pos, Vector3 vel, Vector3 rot, Vector3 spin,
                  Vector3 size, float life, float bounce);
void spawn_opening_burst(Vector3 pos, Vector3 intoDir, float w, bool isDoor, float sill, float head,
                         int count, DebrisKind kind, float size, float speed);

void effects_update(float dt);
void effects_draw(void);

#endif // PARTICLES_H
