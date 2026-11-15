#ifndef ZOMBIES_H
#define ZOMBIES_H

#include "raylib.h"
#include "world/openings.h"
#include <stdbool.h>

#define MAX_ZOMBIES 64
#define MAX_CORPSES 128

#define Z_REACH 1.2f
#define Z_LET_GO 1.5f
#define Z_CLOSE 0.95f
#define Z_SPACE 0.8f
#define Z_SWING 0.85f
#define Z_STRIKE 0.2f
#define Z_HIT_RANGE 1.65f
#define Z_TRACK 7.0f
#define HURT_IFRAMES 1.5f

typedef struct {
    bool active;
    int id;
    int kit;
    Vector3 pos;
    float rotY;
    float rotX;
    float scale;

    float hp;
    float pace;
    float speed;
    float hurt;
    float phase;
    float seed;
    float limp;
    float limpSide;
    float hunch;
    float tilt;
    int arms;
    float flinch;
    float attack;
    float groan;
    float gait;

    float swing;
    float swingCd;
    bool struck;
    bool standing;
    Opening *swingAt;

    float climb;
    Opening *climbO;
    Vector3 climbFrom;

    int via;
    float stuckT;
    float stuckX, stuckZ;
    Opening *opening;
    int stage;
    float heading;
    float turn;

    // Title screen wandering
    Vector2 cross;
    bool hasStopX;
    float stopX;
    float idle;

    // Computed transforms
    Matrix mPelvis;
    Matrix mUpper;
    Matrix mHead;
    Matrix mJaw;
    Matrix mUpperArm[2];
    Matrix mForearm[2];
    Matrix mThigh[2];
    Matrix mShin[2];
    Matrix mFoot[2];
} Zombie;

typedef struct {
    bool active;
    Zombie z; // reuses pose & transforms while toppling
    float deadT;
    int fallDir;
    float restY;
    bool sinking;
    float sinkT;
    Vector2 points[3];
    Vector3 poolPos;
    float poolRotY;
    float poolSize;
    float poolScale;
} Corpse;

extern Zombie gZombies[MAX_ZOMBIES];
extern int gZombieCount;
extern Corpse gCorpses[MAX_CORPSES];

typedef bool (*PlayerHurtCallback)(void);
void zombies_set_hurt_handler(PlayerHurtCallback cb);

void zombies_init(void);
void zombies_clear(void);
void zombies_cleanup(void);

Zombie *zombies_spawn(const Vector3 *optPos);
void zombies_hit(Zombie *z);
void zombies_kill(Zombie *z, Vector3 dir);
void zombies_animate(Zombie *z, float dt, float nearDist, float dead, float sag);
void zombies_chase(Zombie *z, float dt);
void zombies_wander(Zombie *z, float dt);
void zombies_separate(void);
void zombies_update(float dt, bool running, bool titling);
void zombies_draw(void);

Vector3 zombie_spawn_point(void);
bool wall_between(Vector3 a, Vector3 b);
void walk_toward(Zombie *z, float dx, float dz, float speed, float turn, float dt);

#endif // ZOMBIES_H
