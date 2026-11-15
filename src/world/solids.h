#ifndef SOLIDS_H
#define SOLIDS_H

#include "raylib.h"
#include <stdbool.h>

#define MAX_SOLIDS 16
#define MAX_ROUND_NODES (MAX_SOLIDS * 4)
#define Z_RADIUS 0.4f
#define ROUND_PAD 0.35f

typedef struct {
    float x, z, hx, hz;
    float c, s; // cos(ry), sin(ry)
} SolidBox;

typedef struct {
    float x, z;
} NavNode;

extern SolidBox gSolids[MAX_SOLIDS];
extern int gSolidCount;
extern NavNode gRoundNodes[MAX_ROUND_NODES];
extern int gRoundNodeCount;

void solids_clear(void);
void add_solid(float x, float z, float ry, float hx, float hz);
void push_out_of_solids(Vector3 *pos);
bool seg_hits_box(float ax, float az, float bx, float bz, float hx, float hz);
bool clear_path(float ax, float az, float bx, float bz);
void init_solids_nav(void);
Vector2 route_around(Vector3 pos, float tx, float tz, int *via);
bool is_inside_cabin(Vector3 pos, float half);

#endif // SOLIDS_H
