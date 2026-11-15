#ifndef OPENINGS_H
#define OPENINGS_H

#include "raylib.h"
#include "core/blocks.h"
#include <stdbool.h>

#define MAX_OPENINGS 8
#define OPENING_INSET 1.5f
#define DOOR_TOP 2.03f
#define JAMB 0.14f
#define DOOR_HP 12
#define WINDOW_HP 2
#define CLIMB_TIME 1.1f

typedef enum {
    OPENING_DOOR,
    OPENING_WINDOW
} OpeningKind;

typedef struct {
    float minX, maxX, minZ, maxZ;
} Box2D;

typedef struct {
    OpeningKind kind;
    char axis; // 'x' or 'z'
    float u0, u1, w;
    float x, z;
    float ox, oz; // outer waypoint
    float ix, iz; // inner waypoint
    float fx, fz; // front stand point
    float lx, lz; // landing point
    float sill, head;
    float rotY;
    Box2D box;
    int maxHp, hp;
    int climberIdx; // zombie index or -1
    float jolt, fallT;

    // Rendering meshes/models
    Model leafModel;
    Model glassModel;
    Model barsModel;
    Model shardsModel;
    float leafRotY;
    float fallRotX;
    Vector3 fallPos;
    bool glassVisible;
    bool glassCracked;
    bool barsVisible;
    bool shardsVisible;
    float paneZ;
} Opening;

extern Opening gOpenings[MAX_OPENINGS];
extern int gOpeningCount;

Opening *make_opening(char axis, float fixed, float u0, float u1, OpeningKind kind, float wallT);
void fit_opening(Opening *o, BlockBatch *wallB, SeededRng *wrng, Shader shader);
void reset_openings(void);
void update_openings(float dt);
void draw_openings(Shader shader);
void unload_openings(void);
Opening *shut_at(float x, float z);
Opening *nearest_opening(Vector3 pos, const Opening *skip);
void hit_opening(Opening *o);
void break_opening(Opening *o);

#endif // OPENINGS_H
