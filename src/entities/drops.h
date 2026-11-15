#ifndef DROPS_H
#define DROPS_H

#include "raylib.h"
#include <stdbool.h>

#define MAX_DROPS 16
#define DROP_CHANCE 0.05f
#define DROP_PITY 30
#define DROP_LIFE 12.0f

typedef enum {
    DROP_MED = 0,
    DROP_DMG = 1,
    DROP_AMMO = 2
} DropKind;

typedef void (*DropCollectCallback)(DropKind kind, const char *name, Color color);
void drops_set_collect_callback(DropCollectCallback cb);

void drops_init(void);
void drops_clear(void);
void drops_cleanup(void);
void drops_maybe_spawn(Vector3 pos);
void drops_update(float dt);
void drops_draw(void);

#endif // DROPS_H
