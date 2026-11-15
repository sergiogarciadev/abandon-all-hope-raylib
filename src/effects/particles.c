#include "particles.h"
#include "core/math_util.h"
#include "core/blocks.h"
#include "core/scene.h"
#include "rlgl.h"
#include <math.h>
#include <string.h>

typedef struct {
    bool active;
    Vector3 pos;
    Vector3 vel;
    float size;
    float life;
} BloodParticle;

typedef struct {
    bool active;
    DebrisKind kind;
    Vector3 pos;
    Vector3 vel;
    Vector3 rot;
    Vector3 spin;
    Vector3 size;
    float life;
    float maxLife;
    float bounce;
} DebrisItem;

static BloodParticle gBlood[MAX_BLOOD_PARTICLES];
static DebrisItem gDebris[MAX_DEBRIS];
static Model gCasingModel;
static Model gBoxModel;

void effects_init(void) {
    memset(gBlood, 0, sizeof(gBlood));
    memset(gDebris, 0, sizeof(gDebris));
    SeededRng rng = rng_seed(33);
    BlockSpec cas = BLKB(0.05f, 0.05f, 0.12f, 0, 0, 0, 0xc9a14a, 0, 0, 0, 0.012f);
    gCasingModel = model_from_blocks(&cas, 1, &rng, false, gScene.litShader);
    BlockSpec box = BLKB(1.0f, 1.0f, 1.0f, 0, 0, 0, 0xffffff, 0, 0, 0, 0.0f);
    gBoxModel = model_from_blocks(&box, 1, &rng, true, gScene.litShader);
}

void effects_clear(void) {
    memset(gBlood, 0, sizeof(gBlood));
    memset(gDebris, 0, sizeof(gDebris));
}

void effects_cleanup(void) {
    UnloadModel(gCasingModel);
    UnloadModel(gBoxModel);
}

void spawn_blood(Vector3 pos) {
    int spawned = 0;
    for (int i = 0; i < MAX_BLOOD_PARTICLES && spawned < 12; i++) {
        if (gBlood[i].active) continue;
        gBlood[i] = (BloodParticle){
            .active = true,
            .pos = { pos.x, 0.8f, pos.z },
            .vel = { rndf(-2.5f, 2.5f), rndf(3.0f, 7.0f), rndf(-2.5f, 2.5f) },
            .size = rndf(0.12f, 0.30f),
            .life = rndf(0.6f, 1.0f),
        };
        spawned++;
    }
}

void spawn_debris(DebrisKind kind, Vector3 pos, Vector3 vel, Vector3 rot, Vector3 spin,
                  Vector3 size, float life, float bounce) {
    int slot = -1;
    for (int i = 0; i < MAX_DEBRIS; i++) {
        if (!gDebris[i].active) { slot = i; break; }
    }
    if (slot < 0) slot = GetRandomValue(0, MAX_DEBRIS - 1);
    gDebris[slot] = (DebrisItem){
        .active = true,
        .kind = kind,
        .pos = pos,
        .vel = vel,
        .rot = rot,
        .spin = spin,
        .size = size,
        .life = life,
        .maxLife = life,
        .bounce = bounce,
    };
}

void spawn_opening_burst(Vector3 pos, Vector3 intoDir, float w, bool isDoor, float sill, float head,
                         int count, DebrisKind kind, float size, float speed) {
    for (int i = 0; i < count; i++) {
        float along = rndf(-0.5f, 0.5f) * (w - 0.3f);
        bool alongX = fabsf(intoDir.z) > fabsf(intoDir.x);
        Vector3 p = {
            pos.x + (alongX ? along : 0.0f),
            isDoor ? rndf(0.3f, 1.8f) : rndf(sill + 0.05f, head - 0.05f),
            pos.z + (alongX ? 0.0f : along)
        };
        float v = speed * rndf(0.5f, 1.2f);
        Vector3 vel = {
            intoDir.x * v + rndf(-1.0f, 1.0f),
            rndf(0.5f, 2.5f),
            intoDir.z * v + rndf(-1.0f, 1.0f)
        };
        Vector3 sz = { size * rndf(0.4f, 1.0f), size * rndf(0.2f, 0.6f), size * rndf(1.0f, 2.5f) };
        Vector3 rot = { rndf(0, 3), rndf(0, 3), rndf(0, 3) };
        Vector3 spin = { rndf(-12, 12), rndf(-12, 12), rndf(-12, 12) };
        spawn_debris(kind, p, vel, rot, spin, sz, rndf(2.0f, 3.5f), 0.02f);
    }
}

void effects_update(float dt) {
    for (int i = 0; i < MAX_BLOOD_PARTICLES; i++) {
        BloodParticle *p = &gBlood[i];
        if (!p->active) continue;
        p->life -= dt;
        p->vel.y -= 16.0f * dt;
        p->pos = Vector3Add(p->pos, Vector3Scale(p->vel, dt));
        if (p->pos.y < 0.05f) {
            p->pos.y = 0.05f;
            p->vel = (Vector3){0};
        }
        if (p->life <= 0.0f) p->active = false;
    }

    for (int i = 0; i < MAX_DEBRIS; i++) {
        DebrisItem *d = &gDebris[i];
        if (!d->active) continue;
        d->life -= dt;
        if (d->kind == DEBRIS_SMOKE) {
            float drag = expf(-3.0f * dt);
            d->vel = Vector3Scale(d->vel, drag);
            d->pos = Vector3Add(d->pos, Vector3Scale(d->vel, dt));
            d->pos.y += 0.5f * dt;
        } else {
            d->vel.y -= 16.0f * dt;
            d->pos = Vector3Add(d->pos, Vector3Scale(d->vel, dt));
            d->rot.x += d->spin.x * dt;
            d->rot.y += d->spin.y * dt;
            d->rot.z += d->spin.z * dt;
            if (d->pos.y < d->bounce) {
                d->pos.y = d->bounce;
                d->vel.x *= 0.45f;
                d->vel.z *= 0.45f;
                d->vel.y = fabsf(d->vel.y) > 1.5f ? -d->vel.y * 0.35f : 0.0f;
                d->spin = Vector3Scale(d->spin, 0.4f);
                if (d->vel.y == 0.0f) {
                    d->spin = (Vector3){0};
                    d->rot.x = 0.0f;
                    d->rot.z = roundf(d->rot.z / PI) * PI;
                }
            }
        }
        if (d->life <= 0.0f) d->active = false;
    }
}

void effects_draw(void) {
    // Blood cubes
    for (int i = 0; i < MAX_BLOOD_PARTICLES; i++) {
        BloodParticle *p = &gBlood[i];
        if (!p->active) continue;
        Matrix m = MatrixMultiply(MatrixScale(p->size, p->size, p->size),
                                  MatrixTranslate(p->pos.x, p->pos.y, p->pos.z));
        gBoxModel.materials[0].maps[MATERIAL_MAP_DIFFUSE].color = hex_color(0x6a0e0e, 255);
        DrawMesh(gBoxModel.meshes[0], gBoxModel.materials[0], m);
    }

    // Solid debris
    for (int i = 0; i < MAX_DEBRIS; i++) {
        DebrisItem *d = &gDebris[i];
        if (!d->active || d->kind == DEBRIS_SMOKE) continue;
        float shrink = d->life < 0.4f ? fmaxf(0.01f, d->life / 0.4f) : 1.0f;
        Matrix ms = MatrixScale(d->size.x * shrink, d->size.y * shrink, d->size.z * shrink);
        Matrix mr = make_transform_yxz(d->pos.x, d->pos.y, d->pos.z, d->rot.x, d->rot.y, d->rot.z);
        Matrix m = MatrixMultiply(ms, mr);

        if (d->kind == DEBRIS_CASING) {
            DrawMesh(gCasingModel.meshes[0], gCasingModel.materials[0], mr);
        } else {
            Color c = WHITE;
            if (d->kind == DEBRIS_MAG) c = hex_color(0x2b2e34, 255);
            else if (d->kind == DEBRIS_WOOD) c = hex_color(0x6d6352, 255);
            else if (d->kind == DEBRIS_GLASS) c = (Color){ 159, 182, 201, 140 };
            gBoxModel.materials[0].maps[MATERIAL_MAP_DIFFUSE].color = c;
            DrawMesh(gBoxModel.meshes[0], gBoxModel.materials[0], m);
        }
    }
    gBoxModel.materials[0].maps[MATERIAL_MAP_DIFFUSE].color = WHITE;

    // Gun smoke (transparent)
    for (int i = 0; i < MAX_DEBRIS; i++) {
        DebrisItem *d = &gDebris[i];
        if (!d->active || d->kind != DEBRIS_SMOKE) continue;
        float k = 1.0f - d->life / d->maxLife;
        float s = d->size.x * (1.0f + k * 2.5f);
        uint8_t a = (uint8_t)(clampf(0.32f * (1.0f - k) * (1.0f - k), 0.0f, 1.0f) * 255.0f);
        DrawCube(d->pos, s, s, s, (Color){ 141, 143, 148, a });
    }
}
