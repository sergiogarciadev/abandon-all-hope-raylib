#ifndef WORLD_H
#define WORLD_H

#include "raylib.h"
#include "openings.h"

#define WORLD_HALF 10.0f
#define WALL_H 2.4f
#define WALL_T 0.7f
#define WORLD_EDGE (WORLD_HALF + WALL_T * 0.5f)
#define GROUND_Y -0.06f

#define MAX_WALL_BOXES 32
#define MAX_CLEARINGS 16
#define FOREST_COUNT 900
#define BRUSH_COUNT 1000

typedef struct {
    float x, z, r;
} Clearing;

typedef struct {
    Matrix transform;
    Color tint;
    int kind;
    float x, z;
    bool far;
} PropInstance;

typedef struct {
    Box2D wallBoxes[MAX_WALL_BOXES];
    int wallBoxCount;

    Clearing clearings[MAX_CLEARINGS];
    int clearingCount;

    Texture2D groundTex;
    Model groundModel;
    Model floorBaseModel;
    Model floorModel;
    Model wallModel;
    Model yardModel;
    Model lanternGlassModel;

    Model treeModelsNear[5];
    Model treeModelsFar[5];
    PropInstance trees[FOREST_COUNT];
    int treeCount;

    Model brushModels[3];
    PropInstance brush[BRUSH_COUNT];
    int brushCount;

    Texture2D moonTex;
    Vector3 lanternPos;
    float lanternIntensity;
} GameWorld;

extern GameWorld gWorld;

void world_init(void);
void world_update(float dt, float weatherLevel);
void world_draw(Vector3 camTarget);
void world_cleanup(void);

#endif // WORLD_H
