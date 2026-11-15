#include "zombies.h"
#include "animation.h"
#include "player.h"
#include "core/blocks.h"
#include "core/math_util.h"
#include "core/scene.h"
#include "core/state.h"
#include "world/solids.h"
#include "world/world.h"
#include "effects/particles.h"
#include "raymath.h"
#include <math.h>
#include <string.h>

#define Z_THIGH 0.42f
#define Z_SHIN 0.4f
#define Z_ANKLE 0.11f
#define Z_HIP 0.9f
#define Z_UPPER_ARM 0.4f
#define Z_FOREARM 0.39f
#define Z_SCALE 1.08f
#define Z_UPPER 1.1f

#define CORPSE_LIFE 20.0f
#define BODY_THICK 0.3f

typedef struct {
    Model head, jaw, eyes, torso, pelvis, upperArm, forearm, thigh, shin, feet[2];
} ZombieKit;

typedef struct {
    uint32_t shirt, pants, skin, hair, shoe;
    bool bald, tank, ribs, barefoot;
} ZombieLook;

static const ZombieLook ZOMBIE_LOOKS[4] = {
    { 0x4d6680, 0x5a4632, 0x7f9a66, 0x2a2620, 0x2b2420, false, false, false, false },
    { 0x5b6239, 0x6b5f43, 0x8aa67a, 0x4a3a28, 0x3a2c1f, false, false, true,  false },
    { 0x9a9584, 0x3a4a63, 0x6f8a52, 0x000000, 0x1c1c1e, true,  true,  false, true  },
    { 0x86703a, 0x38383b, 0x82a07c, 0x1b1714, 0x242226, false, false, true,  false },
};

static ZombieKit gKits[4];
static Model gBloodPoolModel;
Zombie gZombies[MAX_ZOMBIES];
int gZombieCount = 0;
Corpse gCorpses[MAX_CORPSES];
static int gNextZombieId = 1;
static PlayerHurtCallback gHurtCb = NULL;

void zombies_set_hurt_handler(PlayerHurtCallback cb) {
    gHurtCb = cb;
}

static uint32_t darker_hex(uint32_t hex, float k) {
    Color c = scale_color(hex_color(hex, 255), k);
    return ((uint32_t)c.r << 16) | ((uint32_t)c.g << 8) | (uint32_t)c.b;
}

static void bake_zombie_kit(ZombieKit *kit, const ZombieLook *L, uint32_t seed) {
    SeededRng rng = rng_seed(seed);
    uint32_t skinDk = darker_hex(L->skin, 0.72f);
    uint32_t shirtDk = darker_hex(L->shirt, 0.65f);
    uint32_t pantsDk = darker_hex(L->pants, 0.65f);
    uint32_t sleeve = L->tank ? L->skin : L->shirt;
    const uint32_t GORE = 0x5c1010, BLOOD = 0x3a0807, BONE = 0xcfc4a2, SOCKET = 0x120a08;

    BlockSpec head[16];
    int hc = 0;
    head[hc++] = BLK(0.46f, 0.44f, 0.44f, 0, 0.24f, 0, L->skin);
    head[hc++] = BLK(0.47f, 0.06f, 0.07f, 0, 0.33f, 0.2f, skinDk);
    head[hc++] = BLK(0.11f, 0.08f, 0.03f, -0.11f, 0.27f, 0.215f, SOCKET);
    head[hc++] = BLK(0.11f, 0.08f, 0.03f,  0.11f, 0.27f, 0.215f, SOCKET);
    head[hc++] = BLK(0.06f, 0.07f, 0.04f, 0, 0.2f, 0.23f, skinDk);
    head[hc++] = BLKB(0.25f, 0.03f, 0.03f, 0, 0.06f, 0.225f, BONE, 0, 0, 0, 0.01f);
    head[hc++] = BLK(0.05f, 0.1f, 0.08f, 0.235f, 0.24f, 0, L->skin);
    head[hc++] = BLKB(0.03f, 0.1f, 0.13f, -0.232f, 0.17f, 0.07f, GORE, 0, 0, 0, 0.01f);
    if (L->bald) {
        head[hc++] = BLKB(0.2f, 0.03f, 0.16f, 0.06f, 0.465f, 0.03f, GORE, 0, 0, 0, 0.01f);
        head[hc++] = BLKB(0.09f, 0.03f, 0.08f, 0.07f, 0.475f, 0.03f, BONE, 0, 0, 0, 0.01f);
    } else {
        head[hc++] = BLK(0.44f, 0.16f, 0.08f, 0, 0.32f, -0.2f, L->hair);
        for (int i = 0; i < 3; i++) {
            head[hc++] = BLK(0.12f + rng_next(&rng) * 0.08f, 0.06f, 0.12f + rng_next(&rng) * 0.08f,
                             (rng_next(&rng) - 0.5f) * 0.28f, 0.48f, (rng_next(&rng) - 0.6f) * 0.3f, L->hair);
        }
    }
    kit->head = model_from_blocks(head, hc, &rng, false, gScene.litShader);

    BlockSpec torso[16];
    int tc = 0;
    torso[tc++] = BLK(0.64f, 0.6f, 0.36f, 0, 0.32f, 0, L->shirt);
    torso[tc++] = BLK(0.7f, 0.18f, 0.38f, 0, 0.56f, -0.01f, L->tank ? L->skin : L->shirt);
    torso[tc++] = BLK(0.4f, 0.2f, 0.12f, 0, 0.5f, -0.2f, L->tank ? L->skin : L->shirt);
    torso[tc++] = BLK(0.2f, 0.12f, 0.2f, 0, 0.67f, 0.04f, L->skin);
    torso[tc++] = BLKR(0.15f, 0.13f, 0.39f, -0.21f, 0.0f, 0, shirtDk, 0, 0, 0.25f);
    torso[tc++] = BLKR(0.12f, 0.1f, 0.39f, 0.19f, -0.01f, 0, shirtDk, 0, 0, -0.2f);
    torso[tc++] = BLK(0.18f, 0.24f, 0.02f, -0.03f, 0.47f, 0.185f, BLOOD);
    torso[tc++] = BLK(0.2f, 0.17f, 0.04f, 0.15f, 0.3f, 0.17f, L->skin);
    torso[tc++] = BLK(0.12f, 0.1f, 0.03f, 0.16f, 0.3f, 0.19f, GORE);
    if (L->tank) {
        torso[tc++] = BLK(0.1f, 0.18f, 0.37f, -0.17f, 0.56f, 0, L->shirt);
        torso[tc++] = BLK(0.1f, 0.18f, 0.37f,  0.17f, 0.56f, 0, L->shirt);
    }
    if (L->ribs) {
        torso[tc++] = BLK(0.26f, 0.26f, 0.04f, -0.15f, 0.33f, 0.175f, GORE);
        for (int k = 0; k < 3; k++) {
            torso[tc++] = BLKB(0.24f, 0.035f, 0.03f, -0.15f, 0.25f + k * 0.08f, 0.2f, BONE, 0, 0, 0, 0.01f);
        }
    }
    kit->torso = model_from_blocks(torso, tc, &rng, false, gScene.litShader);

    BlockSpec pelvis[3] = {
        BLK(0.52f, 0.26f, 0.32f, 0, 0.02f, 0, L->pants),
        BLKB(0.54f, 0.06f, 0.34f, 0, 0.13f, 0, 0x2a221a, 0, 0, 0, 0.015f),
        BLKR(0.18f, 0.16f, 0.04f, 0.12f, 0.02f, 0.17f, L->tank ? L->shirt : shirtDk, 0, 0, 0.12f),
    };
    kit->pelvis = model_from_blocks(pelvis, 3, &rng, false, gScene.litShader);

    BlockSpec upArm[2] = {
        BLK(0.17f, Z_UPPER_ARM + 0.06f, 0.18f, 0, 0, 0, sleeve),
        BLK(0.19f, 0.07f, 0.2f, 0, -Z_UPPER_ARM * 0.5f + 0.03f, 0, shirtDk),
    };
    kit->upperArm = model_from_blocks(upArm, L->tank ? 1 : 2, &rng, false, gScene.litShader);

    float handY = -Z_FOREARM * 0.5f - 0.06f;
    BlockSpec fore[6] = {
        BLK(0.15f, Z_FOREARM + 0.04f, 0.16f, 0, 0, 0, L->skin),
        BLKB(0.03f, 0.12f, 0.1f, 0.075f, 0.05f, 0, GORE, 0, 0, 0, 0.01f),
        BLK(0.16f, 0.14f, 0.1f, 0, handY, 0.01f, skinDk),
    };
    const float fx[3] = { -0.05f, 0.0f, 0.05f };
    for (int i = 0; i < 3; i++) {
        fore[3 + i] = BLKB(0.035f, 0.12f, 0.035f, fx[i], handY - 0.12f, 0.02f + rng_next(&rng) * 0.02f,
                           skinDk, 0.2f + rng_next(&rng) * 0.3f, 0, 0, 0.008f);
    }
    kit->forearm = model_from_blocks(fore, 6, &rng, false, gScene.litShader);

    BlockSpec thigh = BLK(0.21f, Z_THIGH + 0.06f, 0.23f, 0, 0, 0, L->pants);
    kit->thigh = model_from_blocks(&thigh, 1, &rng, false, gScene.litShader);

    BlockSpec shin[2] = {
        BLK(0.19f, Z_SHIN + 0.05f, 0.21f, 0, 0, 0, L->pants),
        BLK(0.21f, 0.07f, 0.23f, 0, -Z_SHIN * 0.5f + 0.02f, 0, pantsDk),
    };
    kit->shin = model_from_blocks(shin, 2, &rng, false, gScene.litShader);

    BlockSpec shoe[2] = {
        BLK(0.22f, 0.12f, 0.34f, 0, -0.05f, 0.05f, L->shoe),
        BLKB(0.23f, 0.03f, 0.35f, 0, -0.1f, 0.05f, 0x141210, 0, 0, 0, 0.01f),
    };
    BlockSpec bare[2] = {
        BLK(0.18f, 0.08f, 0.3f, 0, -0.07f, 0.06f, skinDk),
        BLK(0.16f, 0.05f, 0.06f, 0, -0.09f, 0.22f, skinDk),
    };
    kit->feet[0] = model_from_blocks(shoe, 2, &rng, false, gScene.litShader);
    kit->feet[1] = model_from_blocks(L->barefoot ? bare : shoe, 2, &rng, false, gScene.litShader);

    BlockSpec eyes[2] = {
        BLKB(0.06f, 0.045f, 0.02f, -0.11f, 0.27f, 0.225f, 0xd9e06a, 0, 0, 0, 0.005f),
        BLKB(0.06f, 0.045f, 0.02f,  0.11f, 0.27f, 0.225f, 0xd9e06a, 0, 0, 0, 0.005f),
    };
    kit->eyes = model_from_blocks(eyes, 2, &rng, false, gScene.litShader);

    BlockSpec jaw[3] = {
        BLK(0.36f, 0.09f, 0.3f, 0, -0.015f, 0.15f, L->skin),
        BLKB(0.22f, 0.025f, 0.03f, 0, 0.035f, 0.29f, BONE, 0, 0, 0, 0.01f),
        BLKB(0.2f, 0.05f, 0.02f, 0, -0.035f, 0.305f, BLOOD, 0, 0, 0, 0.01f),
    };
    kit->jaw = model_from_blocks(jaw, 3, &rng, false, gScene.litShader);
}

void zombies_init(void) {
    memset(gZombies, 0, sizeof(gZombies));
    memset(gCorpses, 0, sizeof(gCorpses));
    gZombieCount = 0;

    for (int i = 0; i < 4; i++) {
        bake_zombie_kit(&gKits[i], &ZOMBIE_LOOKS[i], (uint32_t)(7 + i * 31));
    }

    SeededRng prng = rng_seed(99);
    BlockSpec slabs[7];
    slabs[0] = BLKB(0.8f, 0.02f, 0.8f, 0, 0, 0, 0x3a0606, 0, 0.3f, 0, 0.008f);
    for (int i = 0; i < 6; i++) {
        float a = (float)i * 1.1f + rng_next(&prng);
        float r = 0.3f + rng_next(&prng) * 0.25f;
        float s = 0.25f + rng_next(&prng) * 0.3f;
        slabs[1 + i] = BLKB(s, 0.02f, s * (0.6f + rng_next(&prng) * 0.6f),
                            cosf(a) * r, 0.003f * i, sinf(a) * r, 0x3a0606,
                            0, rng_next(&prng) * 3.0f, 0, 0.008f);
    }
    gBloodPoolModel = model_from_blocks(slabs, 7, &prng, false, gScene.litShader);
}

void zombies_clear(void) {
    memset(gZombies, 0, sizeof(gZombies));
    memset(gCorpses, 0, sizeof(gCorpses));
    gZombieCount = 0;
}

void zombies_cleanup(void) {
    for (int i = 0; i < 4; i++) {
        UnloadModel(gKits[i].head);
        UnloadModel(gKits[i].jaw);
        UnloadModel(gKits[i].eyes);
        UnloadModel(gKits[i].torso);
        UnloadModel(gKits[i].pelvis);
        UnloadModel(gKits[i].upperArm);
        UnloadModel(gKits[i].forearm);
        UnloadModel(gKits[i].thigh);
        UnloadModel(gKits[i].shin);
        UnloadModel(gKits[i].feet[0]);
        UnloadModel(gKits[i].feet[1]);
    }
    UnloadModel(gBloodPoolModel);
}

static bool offscreen(float x, float z, Camera3D cam) {
    int sw = GetScreenWidth(), sh = GetScreenHeight();
    for (int yi = 0; yi < 2; yi++) {
        float y = yi ? 2.2f : 0.0f;
        Vector2 sp = GetWorldToScreen((Vector3){ x, y, z }, cam);
        if (sp.x >= -sw * 0.05f && sp.x <= sw * 1.05f && sp.y >= -sh * 0.05f && sp.y <= sh * 1.05f) {
            return false;
        }
    }
    return true;
}

Vector3 zombie_spawn_point(void) {
    Camera3D spawnCam = gScene.camera;
    spawnCam.position = (Vector3){ gPlayer.position.x + CAM_OFFSET_X, CAM_OFFSET_Y, gPlayer.position.z + CAM_OFFSET_Z };
    spawnCam.target = (Vector3){ gPlayer.position.x, 0.0f, gPlayer.position.z };

    float ang = rndf(0.0f, 2.0f * PI);
    float cx = sinf(ang), cz = cosf(ang);
    float r = 12.0f;
    for (; r < 70.0f; r += 1.0f) {
        float x = gPlayer.position.x + cx * r;
        float z = gPlayer.position.z + cz * r;
        if (offscreen(x, z, spawnCam) && (fabsf(x) > WORLD_EDGE + 1.5f || fabsf(z) > WORLD_EDGE + 1.5f)) break;
    }
    float rf = r + rndf(1.5f, 4.5f);
    Vector3 out = { gPlayer.position.x + cx * rf, 0.0f, gPlayer.position.z + cz * rf };
    push_out_of_solids(&out);
    return out;
}

Zombie *zombies_spawn(const Vector3 *optPos) {
    int slot = -1;
    for (int i = 0; i < MAX_ZOMBIES; i++) {
        if (!gZombies[i].active) { slot = i; break; }
    }
    if (slot < 0) return NULL;

    Vector3 pos = optPos ? *optPos : zombie_spawn_point();
    Zombie *z = &gZombies[slot];
    *z = (Zombie){0};
    z->active = true;
    z->id = gNextZombieId++;
    z->kit = GetRandomValue(0, 3);
    z->pos = pos;
    z->scale = Z_SCALE * rndf(0.94f, 1.06f);
    z->hp = gState.zombieHp;
    z->pace = rndf(0.85f, 1.15f);
    z->speed = gState.zombieSpeed * z->pace;
    z->phase = rnd01();
    z->seed = rndf(0.0f, 100.0f);
    z->limp = rnd01() < 0.45f ? rndf(0.5f, 1.0f) : 0.0f;
    z->limpSide = rnd01() < 0.5f ? 1.0f : -1.0f;
    z->hunch = rndf(0.25f, 0.55f);
    z->tilt = rndf(-0.25f, 0.25f);
    float ar = rnd01();
    z->arms = ar < 0.7f ? 2 : (ar < 0.85f ? 1 : 0);
    z->gait = 1.0f;
    z->swing = -1.0f;
    z->swingCd = 0.15f;
    z->climb = -1.0f;
    z->via = -1;
    z->stuckX = pos.x;
    z->stuckZ = pos.z;
    z->heading = atan2f(-pos.x, -pos.z);
    z->rotY = z->heading;
    z->turn = rndf(3.0f, 5.0f);
    gZombieCount++;
    return z;
}

void zombies_hit(Zombie *z) {
    z->hurt = 0.12f;
    z->flinch = 1.0f;
}

static Vector3 xform_pt(Vector3 p, Matrix m) {
    return (Vector3){
        m.m0 * p.x + m.m4 * p.y + m.m8  * p.z + m.m12,
        m.m1 * p.x + m.m5 * p.y + m.m9  * p.z + m.m13,
        m.m2 * p.x + m.m6 * p.y + m.m10 * p.z + m.m14
    };
}

void zombies_animate(Zombie *z, float dt, float nearDist, float dead, float sag) {
    int fall = dead > 0.0f ? 1 : 0; // fallDir handled in corpse transform
    float live = 1.0f - dead;
    float gait = z->gait * live;
    float sp = z->speed * gait;
    float run = clampf((z->speed - 3.0f) / 2.0f, 0.0f, 1.0f);
    float freq = 1.1f + sp * 0.22f;
    float beta = 0.6f - 0.18f * run;
    z->phase = fmodf(z->phase + freq * dt * live, 1.0f);
    float ph = z->phase * 2.0f * PI;
    float L = fminf(0.7f, sp * beta / freq);
    float t = (float)GetTime() + z->seed;

    z->flinch *= expf(-7.0f * dt);
    z->groan  *= expf(-2.5f * dt);
    z->attack = dampf(z->attack, (nearDist < 2.4f && z->swing < 0.0f) ? live : 0.0f, 6.0f, dt);

    float sw = z->swing;
    float rec = Z_SWING - Z_STRIKE - 0.1f;
    float blow = 0.0f;
    if (sw >= 0.0f && dead <= 0.0f) {
        if (sw < Z_STRIKE) blow = -ease_smooth(sw / Z_STRIKE);
        else if (sw < Z_STRIKE + 0.1f) blow = -1.0f + 2.0f * ease_smooth((sw - Z_STRIKE) / 0.1f);
        else blow = 1.0f - ease_smooth(fminf(1.0f, (sw - Z_STRIKE - 0.1f) / rec));
    }
    float rear = fmaxf(0.0f, -blow);
    float slam = fmaxf(0.0f, blow);
    float clamber = (z->climb >= 0.0f && dead <= 0.0f) ? sinf(PI * z->climb) : 0.0f;

    Matrix mRoot = MatrixMultiply(
        MatrixScale(z->scale, z->scale, z->scale),
        make_transform_yxz(z->pos.x, z->pos.y, z->pos.z, z->rotX, z->rotY, 0.0f)
    );

    float badStance = z->limp > 0.0f ? fmaxf(0.0f, sinf(ph + (z->limpSide > 0.0f ? 0.0f : PI))) : 0.0f;
    float pelvisY = Z_HIP - 0.05f - 0.06f * run - 0.05f * fabsf(sinf(ph)) * gait
                    - 0.06f * slam - 0.08f * z->limp * badStance - 0.3f * sag;
    Matrix mPelvisLocal = make_transform_yxz(
        0.0f, pelvisY, 0.0f,
        0.0f, 0.12f * sinf(ph) * live,
        (0.1f * sinf(ph) + 0.12f * z->limp * badStance * z->limpSide) * live
    );
    z->mPelvis = MatrixMultiply(mPelvisLocal, mRoot);

    Matrix mUpperLocal = MatrixMultiply(
        MatrixScale(Z_UPPER, Z_UPPER, Z_UPPER),
        make_transform_yxz(
            0.0f, pelvisY + 0.08f, 0.0f,
            (z->hunch + 0.12f * run + 0.25f * z->attack - 0.55f * rear + 0.5f * slam) * live - 0.6f * z->flinch + 0.35f * fall * dead,
            -0.15f * sinf(ph) * live,
            (-0.14f * sinf(ph) - 0.1f * z->limp * badStance * z->limpSide) * live + 0.04f * sinf(t * 0.7f)
        )
    );
    z->mUpper = MatrixMultiply(mUpperLocal, mRoot);

    Matrix mHeadLocal = make_transform_yxz(
        0.0f, 0.68f, 0.06f,
        (-z->hunch * 0.7f + 0.12f * fabsf(sinf(ph)) - 0.25f * z->attack - 0.2f * rear) * live - 0.7f * z->flinch - 0.3f * dead,
        0.25f * sinf(t * 0.9f) * live,
        z->tilt + 0.18f * sinf(t * 1.3f + 1.0f) * live
    );
    z->mHead = MatrixMultiply(mHeadLocal, z->mUpper);

    Matrix mJawLocal = make_transform_yxz(
        0.0f, 0.0f, -0.06f,
        (0.12f + 0.12f * fabsf(sinf(t * 2.3f)) + 0.45f * z->groan + 0.35f * z->attack * fabsf(sinf(t * 9.0f)) + 0.5f * rear) * live + 0.5f * dead,
        0.0f, 0.0f
    );
    z->mJaw = MatrixMultiply(mJawLocal, z->mHead);

    const Vector3 kneePole = { 0, 0, 1 };
    for (int i = 0; i < 2; i++) {
        float side = (i == 0) ? 1.0f : -1.0f;
        bool bad = z->limp > 0.0f && side == z->limpSide;
        float p = fmodf(z->phase + (side > 0.0f ? 0.0f : 0.5f + 0.06f * z->limp), 1.0f);
        FootPose fp = foot_cycle(p, beta);

        Vector3 foot3 = {
            side * (0.15f + 0.06f * dead),
            Z_ANKLE + fp.lift * (bad ? 0.03f : 0.12f + 0.1f * run) * gait + 0.3f * clamber,
            fp.s * L - 0.2f * clamber
        };
        Vector3 hipP = xform_pt((Vector3){ side * 0.14f, 0.0f, 0.0f }, mPelvisLocal);
        Vector3 knee = ik2(hipP, foot3, Z_THIGH, Z_SHIN, kneePole);
        Vector3 ankle = Vector3Add(knee, Vector3Scale(Vector3Normalize(Vector3Subtract(foot3, knee)), Z_SHIN));
        z->mThigh[i] = MatrixMultiply(bone_transform(hipP, knee, kneePole), mRoot);
        z->mShin[i]  = MatrixMultiply(bone_transform(knee, ankle, kneePole), mRoot);
        z->mFoot[i]  = MatrixMultiply(
            make_transform_yxz(ankle.x, ankle.y, ankle.z, bad ? 0.35f : fp.pitch * live, side * (bad ? 0.35f : 0.12f), 0.0f),
            mRoot
        );

        Vector3 sh = xform_pt((Vector3){ side * 0.36f, 0.58f, 0.0f }, mUpperLocal);
        bool reaches = (side > 0.0f ? z->arms >= 1 : z->arms >= 2) && dead <= 0.0f;
        float k = side * 1.7f;
        Vector3 reach;
        if (reaches) {
            float claw = z->attack * sinf(t * 9.0f + k);
            reach = (Vector3){
                -side * 0.08f,
                -0.08f + 0.06f * sinf(ph + k) + 0.18f * z->attack + 0.12f * claw - 0.3f * z->flinch,
                0.72f + 0.07f * sinf(ph * 2.0f + k) + 0.2f * claw - 0.35f * z->flinch
            };
        } else if (dead > 0.0f) {
            reach = (Vector3){ side * 0.45f * dead, -0.7f, -0.15f * fall * dead };
        } else {
            reach = (Vector3){ side * 0.1f, -0.84f, 0.1f + 0.16f * sinf(ph + (side > 0.0f ? PI : 0.0f)) * gait + 0.25f * z->flinch };
        }
        if (rear > 0.0f) reach = Vector3Lerp(reach, (Vector3){ side * 0.22f, 0.5f, 0.18f }, rear);
        if (slam > 0.0f) reach = Vector3Lerp(reach, (Vector3){ -side * 0.06f, -0.3f, 0.78f }, slam);
        if (clamber > 0.0f) reach = Vector3Lerp(reach, (Vector3){ side * 0.24f, -0.6f, 0.45f }, clamber);
        reach = Vector3Add(reach, sh);

        Vector3 armPole = { side * 0.6f, -1.0f, -0.3f };
        Vector3 elbow = ik2(sh, reach, Z_UPPER_ARM, Z_FOREARM, armPole);
        z->mUpperArm[i] = MatrixMultiply(bone_transform(sh, elbow, armPole), mRoot);
        z->mForearm[i]  = MatrixMultiply(bone_transform(elbow, reach, armPole), mRoot);
    }
}

static void end_climb(Zombie *z, bool died) {
    Opening *o = z->climbO;
    if (o && o->climberIdx == z->id) o->climberIdx = -1;
    if (died && z->climb < 0.5f) z->pos = z->climbFrom;
    else if (o) z->pos = (Vector3){ o->lx, 0.0f, o->lz };
    z->pos.y = 0.0f;
    z->rotX = 0.0f;
    z->climb = -1.0f;
    z->climbO = NULL;
    z->opening = NULL;
    z->stage = 0;
}

static void start_climb(Zombie *z, Opening *o) {
    o->climberIdx = z->id;
    z->climb = 0.0f;
    z->climbO = o;
    z->climbFrom = z->pos;
    z->heading = atan2f(o->lx - z->pos.x, o->lz - z->pos.z);
    z->rotY = z->heading;
}

static void climb_step(Zombie *z, float dt) {
    Opening *o = z->climbO;
    z->climb = fminf(1.0f, z->climb + dt / CLIMB_TIME);
    float k = z->climb, e = ease_smooth(k);
    z->pos.x = z->climbFrom.x + (o->lx - z->climbFrom.x) * e;
    z->pos.z = z->climbFrom.z + (o->lz - z->climbFrom.z) * e;
    z->pos.y = (o->sill - 0.15f) * fminf(1.0f, sinf(PI * k) * 1.4f);
    z->rotX = 0.75f * sinf(PI * k);
    if (k >= 1.0f) end_climb(z, false);
}

void zombies_kill(Zombie *z, Vector3 dir) {
    if (z->climb >= 0.0f) end_climb(z, true);
    spawn_blood(z->pos);

    int slot = -1;
    for (int i = 0; i < MAX_CORPSES; i++) {
        if (!gCorpses[i].active) { slot = i; break; }
    }
    if (slot < 0) {
        // Evict oldest corpse
        float maxDead = -1.0f;
        for (int i = 0; i < MAX_CORPSES; i++) {
            if (gCorpses[i].deadT > maxDead) { maxDead = gCorpses[i].deadT; slot = i; }
        }
    }

    Corpse *c = &gCorpses[slot];
    *c = (Corpse){0};
    c->active = true;
    c->z = *z;
    c->z.hurt = 0.0f;
    float fx = dir.x, fz = dir.z;
    c->fallDir = (fx * sinf(z->heading) + fz * cosf(z->heading) >= 0.0f) ? 1 : -1;

    float dx = sinf(z->heading) * c->fallDir;
    float dz = cosf(z->heading) * c->fallDir;
    const float ptsT[3] = { 0.5f, 1.1f, 1.7f };
    for (int i = 0; i < 3; i++) {
        c->points[i] = (Vector2){ z->pos.x + dx * ptsT[i] * z->scale, z->pos.z + dz * ptsT[i] * z->scale };
    }
    c->restY = 0.0f;
    for (int i = 0; i < MAX_CORPSES; i++) {
        if (i == slot || !gCorpses[i].active || gCorpses[i].sinking) continue;
        bool overlap = false;
        for (int a = 0; a < 3 && !overlap; a++) {
            for (int b = 0; b < 3; b++) {
                float px = c->points[a].x - gCorpses[i].points[b].x;
                float pz = c->points[a].y - gCorpses[i].points[b].y;
                if (px * px + pz * pz < 0.36f) { overlap = true; break; }
            }
        }
        if (overlap) c->restY = fmaxf(c->restY, gCorpses[i].restY + BODY_THICK);
    }
    c->restY = fminf(c->restY, 1.5f);
    c->poolPos = (Vector3){ z->pos.x + dx * 0.9f, 0.045f, z->pos.z + dz * 0.9f };
    c->poolRotY = rndf(0.0f, PI);
    c->poolSize = rndf(1.1f, 1.7f);
    c->poolScale = 0.05f;

    z->active = false;
    gZombieCount--;
    gState.kills++;
}

bool wall_between(Vector3 a, Vector3 b) {
    for (int i = 1; i < 4; i++) {
        float t = (float)i * 0.25f;
        float x = a.x + (b.x - a.x) * t;
        float z = a.z + (b.z - a.z) * t;
        for (int w = 0; w < gWorld.wallBoxCount; w++) {
            Box2D wb = gWorld.wallBoxes[w];
            if (x >= wb.minX && x <= wb.maxX && z >= wb.minZ && z <= wb.maxZ) return true;
        }
        if (shut_at(x, z)) return true;
    }
    return false;
}

static float face_toward(Zombie *z, float dx, float dz, float rate, float dt) {
    float dA = wrap_angle(atan2f(dx, dz) - z->heading);
    float step = clampf(dA, -rate * dt, rate * dt);
    z->heading += step;
    z->rotY = z->heading;
    return fabsf(dA) - fabsf(step);
}

void walk_toward(Zombie *z, float dx, float dz, float speed, float turn, float dt) {
    float off = face_toward(z, dx, dz, turn, dt);
    float v = speed * fmaxf(0.15f, cosf(fminf(off, PI * 0.5f)));
    z->pos.x += sinf(z->heading) * v * dt;
    z->pos.z += cosf(z->heading) * v * dt;
}

void zombies_chase(Zombie *z, float dt) {
    Vector3 p = gPlayer.position;
    float pdx = p.x - z->pos.x, pdz = p.z - z->pos.z;
    float pd = hypotf(pdx, pdz);
    bool walking = false;

    if (z->climb >= 0.0f) {
        climb_step(z, dt);
    } else if (z->swing >= 0.0f) {
        Opening *o = z->swingAt;
        z->swing += dt;
        if (z->swing < Z_STRIKE) {
            face_toward(z, o ? o->x - z->pos.x : pdx, o ? o->z - z->pos.z : pdz, Z_TRACK, dt);
        }
        if (!z->struck && z->swing >= Z_STRIKE) {
            z->struck = true;
            if (o) {
                hit_opening(o);
            } else if (pd < Z_HIT_RANGE && !wall_between(z->pos, p)) {
                if (gHurtCb) gHurtCb();
            }
        }
        if (z->swing >= Z_SWING) {
            z->swing = -1.0f;
            z->swingCd = rndf(0.25f, 0.6f);
        }
    } else if (pd < (z->standing ? Z_LET_GO : Z_REACH) && !wall_between(z->pos, p)) {
        if (!z->standing) z->swingCd = 0.0f;
        z->standing = true;
        float off = face_toward(z, pdx, pdz, Z_TRACK, dt);
        z->swingCd -= dt;
        if (z->swingCd <= 0.0f && off < 0.5f) {
            z->swing = 0.0f;
            z->struck = false;
            z->swingAt = NULL;
        }
    } else {
        z->standing = false;
        bool hasTarget = true;
        float tx = p.x, tz = p.z;
        if (is_inside_cabin(z->pos, WORLD_HALF)) {
            z->opening = NULL;
            z->stage = 0;
        } else {
            if (!z->opening) {
                z->opening = nearest_opening(z->pos, NULL);
                z->stage = 0;
            }
            Opening *o = z->opening;
            if (o) {
                if (z->stage == 0) {
                    tx = o->ox; tz = o->oz;
                    float dx = o->ox - z->pos.x, dz = o->oz - z->pos.z;
                    if (dx * dx + dz * dz < 1.4f) z->stage = 1;
                }
                if (z->stage == 1) {
                    float dx = o->x - z->pos.x, dz = o->z - z->pos.z;
                    if (o->kind == OPENING_DOOR && o->hp <= 0) {
                        tx = o->ix; tz = o->iz;
                    } else if (dx * dx + dz * dz > 1.45f * 1.45f) {
                        tx = o->fx; tz = o->fz;
                    } else {
                        hasTarget = false;
                        float off = face_toward(z, dx, dz, Z_TRACK, dt);
                        z->swingCd -= dt;
                        if (o->hp > 0) {
                            if (z->swingCd <= 0.0f && off < 0.5f) {
                                z->swing = 0.0f;
                                z->struck = false;
                                z->swingAt = o;
                            }
                        } else if (o->climberIdx < 0) {
                            start_climb(z, o);
                        }
                    }
                }
            }
        }
        if (hasTarget) {
            Vector2 g = route_around(z->pos, tx, tz, &z->via);
            walk_toward(z, g.x - z->pos.x, g.y - z->pos.z, z->speed, z->turn * (pd < 4.0f ? 2.5f : 1.0f), dt);
            walking = true;
        }
    }

    if (walking) {
        z->stuckT += dt;
        if (z->stuckT > 2.0f) {
            if (hypotf(z->pos.x - z->stuckX, z->pos.z - z->stuckZ) < 0.6f && z->opening && !is_inside_cabin(z->pos, WORLD_HALF)) {
                z->opening = nearest_opening(z->pos, z->opening);
                z->stage = 0;
                z->via = -1;
            }
            z->stuckT = 0.0f;
        }
    }
    if (!walking || z->stuckT == 0.0f) {
        z->stuckX = z->pos.x;
        z->stuckZ = z->pos.z;
        if (!walking) z->stuckT = 0.0f;
    }

    z->gait = dampf(z->gait, walking ? 1.0f : 0.0f, 8.0f, dt);
    zombies_animate(z, dt, pd, 0.0f, 0.0f);
}

void zombies_wander(Zombie *z, float dt) {
    float dir = z->cross.x >= 0.0f ? 1.0f : -1.0f;
    bool walking = false;
    float cx = 2.0f + 5.0f * ((float)GetScreenWidth() / (float)GetScreenHeight());

    if (z->idle > 0.0f) {
        z->idle -= dt;
    } else if (z->hasStopX && (z->pos.x - z->stopX) * dir > 0.0f) {
        z->hasStopX = false;
        z->idle = rndf(1.5f, 3.5f);
    } else if (z->pos.x * dir > cx) {
        float lz = rndf(11.5f, 15.5f);
        z->pos = (Vector3){ -dir * cx, 0.0f, lz };
        push_out_of_solids(&z->pos);
        z->cross = (Vector2){ dir * (cx + 1.0f), clampf(lz + rndf(-2.0f, 2.0f), 11.5f, 15.5f) };
        z->hasStopX = rnd01() < 0.35f;
        z->stopX = rndf(-5.0f, 5.0f);
        z->speed = rndf(0.7f, 1.05f);
        z->heading = dir * PI * 0.5f;
        z->rotY = z->heading;
        z->idle = rndf(0.0f, 1.0f);
        z->via = -1;
    } else {
        Vector2 g = route_around(z->pos, z->cross.x, z->cross.y, &z->via);
        walk_toward(z, g.x - z->pos.x, g.y - z->pos.z, z->speed, 1.5f, dt);
        walking = true;
    }
    z->gait = dampf(z->gait, walking ? 1.0f : 0.0f, 4.0f, dt);
    zombies_animate(z, dt, 99.0f, 0.0f, 0.0f);
}

void zombies_separate(void) {
    for (int i = 0; i < MAX_ZOMBIES; i++) {
        if (!gZombies[i].active) continue;
        for (int k = i + 1; k < MAX_ZOMBIES; k++) {
            if (!gZombies[k].active) continue;
            float dx = gZombies[k].pos.x - gZombies[i].pos.x;
            float dz = gZombies[k].pos.z - gZombies[i].pos.z;
            float d2 = dx * dx + dz * dz;
            if (d2 >= Z_SPACE * Z_SPACE || d2 < 1e-6f) continue;
            float d = sqrtf(d2);
            float push = (Z_SPACE - d) * 0.5f / d;
            gZombies[i].pos.x -= dx * push;
            gZombies[i].pos.z -= dz * push;
            gZombies[k].pos.x += dx * push;
            gZombies[k].pos.z += dz * push;
        }
    }
    for (int i = 0; i < MAX_ZOMBIES; i++) {
        Zombie *z = &gZombies[i];
        if (!z->active || z->climb >= 0.0f) continue;
        if (gState.running) {
            float dx = z->pos.x - gPlayer.position.x, dz = z->pos.z - gPlayer.position.z;
            float d = hypotf(dx, dz);
            if (d < Z_CLOSE && d > 1e-6f) {
                z->pos.x += (dx / d) * (Z_CLOSE - d);
                z->pos.z += (dz / d) * (Z_CLOSE - d);
            }
        }
        push_out_of_solids(&z->pos);
    }
}

void zombies_update(float dt, bool running, bool titling) {
    for (int i = 0; i < MAX_ZOMBIES; i++) {
        Zombie *z = &gZombies[i];
        if (!z->active) continue;
        if (z->hurt > 0.0f) z->hurt -= dt;
        if (running) zombies_chase(z, dt);
        else if (titling) zombies_wander(z, dt);
    }
    if (running || titling) zombies_separate();

    for (int i = 0; i < MAX_CORPSES; i++) {
        Corpse *c = &gCorpses[i];
        if (!c->active) continue;
        c->deadT += dt;
        if (!c->sinking && c->deadT > CORPSE_LIFE) {
            c->sinking = true;
            c->sinkT = 0.0f;
        }
        if (c->sinking) {
            c->sinkT += dt;
            c->z.pos.y = (0.12f + c->restY) - c->sinkT * (0.6f + c->restY * 0.5f);
            c->poolPos.y = 0.045f - c->sinkT * 0.04f;
            if (c->z.pos.y < -1.1f - c->restY) c->active = false;
            continue;
        }
        float crumple = fminf(1.0f, c->deadT / 0.25f);
        float fall = fminf(1.0f, fmaxf(0.0f, (c->deadT - 0.1f) / 0.45f));
        float f = fall * fall;
        float bounce = (fall >= 1.0f) ? 0.06f * expf(-(c->deadT - 0.55f) * 9.0f) * sinf((c->deadT - 0.55f) * 30.0f) : 0.0f;
        c->z.rotX = c->fallDir * (f * 1.42f - bounce);
        c->z.pos.y = (0.12f + c->restY) * f;
        c->poolScale = fminf(1.0f, fmaxf(0.05f, (c->deadT - 0.4f) / 1.4f)) * c->poolSize;
        if (c->deadT < 1.1f) {
            zombies_animate(&c->z, dt, 99.0f, crumple, crumple * (1.0f - 0.85f * fall));
        }
    }
}

static void draw_zombie_rig(const Zombie *z, bool drawEyes) {
    const ZombieKit *k = &gKits[z->kit];
    if (z->hurt > 0.0f) {
        float em[3] = { 0.38f, 0.41f, 0.31f };
        SetShaderValue(gScene.litShader, gScene.locEmissive, em, SHADER_UNIFORM_VEC3);
    }

    DrawMesh(k->pelvis.meshes[0], k->pelvis.materials[0], z->mPelvis);
    DrawMesh(k->torso.meshes[0],  k->torso.materials[0],  z->mUpper);
    DrawMesh(k->head.meshes[0],   k->head.materials[0],   z->mHead);
    DrawMesh(k->jaw.meshes[0],    k->jaw.materials[0],    z->mJaw);
    for (int i = 0; i < 2; i++) {
        DrawMesh(k->upperArm.meshes[0], k->upperArm.materials[0], z->mUpperArm[i]);
        DrawMesh(k->forearm.meshes[0],  k->forearm.materials[0],  z->mForearm[i]);
        DrawMesh(k->thigh.meshes[0],    k->thigh.materials[0],    z->mThigh[i]);
        DrawMesh(k->shin.meshes[0],     k->shin.materials[0],     z->mShin[i]);
        DrawMesh(k->feet[i].meshes[0],  k->feet[i].materials[0],  z->mFoot[i]);
    }

    if (drawEyes) {
        float emEye[3] = { 0.55f, 0.58f, 0.25f };
        SetShaderValue(gScene.litShader, gScene.locEmissive, emEye, SHADER_UNIFORM_VEC3);
        DrawMesh(k->eyes.meshes[0], k->eyes.materials[0], z->mHead);
    }

    float zero[3] = { 0, 0, 0 };
    SetShaderValue(gScene.litShader, gScene.locEmissive, zero, SHADER_UNIFORM_VEC3);
}

void zombies_draw(void) {
    for (int i = 0; i < MAX_ZOMBIES; i++) {
        if (gZombies[i].active) draw_zombie_rig(&gZombies[i], true);
    }
    for (int i = 0; i < MAX_CORPSES; i++) {
        Corpse *c = &gCorpses[i];
        if (!c->active) continue;
        draw_zombie_rig(&c->z, false);
        Matrix pm = MatrixMultiply(
            MatrixScale(c->poolScale, 1.0f, c->poolScale),
            make_transform_yxz(c->poolPos.x, c->poolPos.y, c->poolPos.z, 0.0f, c->poolRotY, 0.0f)
        );
        DrawMesh(gBloodPoolModel.meshes[0], gBloodPoolModel.materials[0], pm);
    }
}
