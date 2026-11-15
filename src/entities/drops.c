#include "drops.h"
#include "player.h"
#include "core/blocks.h"
#include "core/math_util.h"
#include "core/scene.h"
#include "core/state.h"
#include "world/world.h"
#include "rlgl.h"
#include <math.h>
#include <string.h>

typedef struct {
    bool active;
    DropKind kind;
    Vector3 pos;
    float rotY;
    float life;
    float t;
    bool visible;
} DropItem;

static DropItem gDrops[MAX_DROPS];
static Model gDropModels[3];
static DropCollectCallback gCollectCb = NULL;

static const char *DROP_NAMES[3] = { "+1 HEALTH", "DOUBLE DAMAGE", "BOTTOMLESS MAG" };
static const uint32_t DROP_COLORS[3] = { 0xd6453c, 0xff7a2e, 0xe8d28a };
static const float DROP_WEIGHTS[3] = { 40.0f, 25.0f, 25.0f };

void drops_set_collect_callback(DropCollectCallback cb) {
    gCollectCb = cb;
}

void drops_init(void) {
    memset(gDrops, 0, sizeof(gDrops));
    SeededRng rng = rng_seed(3);

    BlockSpec med[5] = {
        BLKB(0.52f, 0.34f, 0.36f, 0, 0, 0, 0xe6e2da, 0, 0, 0, 0.05f),
        BLKB(0.3f, 0.02f, 0.1f, 0, 0.175f, 0, 0xc8302a, 0, 0, 0, 0.006f),
        BLKB(0.1f, 0.022f, 0.3f, 0, 0.176f, 0, 0xc8302a, 0, 0, 0, 0.006f),
        BLKB(0.24f, 0.08f, 0.02f, 0, 0, 0.185f, 0xc8302a, 0, 0, 0, 0.006f),
        BLKB(0.08f, 0.22f, 0.022f, 0, 0, 0.186f, 0xc8302a, 0, 0, 0, 0.006f),
    };
    gDropModels[DROP_MED] = model_from_blocks(med, 5, &rng, false, gScene.litShader);

    BlockSpec dmg[6] = {
        BLKB(0.13f, 0.3f, 0.13f, -0.09f, -0.06f, 0, 0xc9a14a, 0, 0, 0, 0.03f),
        BLKB(0.13f, 0.3f, 0.13f,  0.09f, -0.06f, 0, 0xc9a14a, 0, 0, 0, 0.03f),
        BLKB(0.11f, 0.16f, 0.11f, -0.09f, 0.17f, 0, 0xff5a1e, 0, 0, 0, 0.04f),
        BLKB(0.11f, 0.16f, 0.11f,  0.09f, 0.17f, 0, 0xff5a1e, 0, 0, 0, 0.04f),
        BLKB(0.06f, 0.07f, 0.06f, -0.09f, 0.27f, 0, 0xff9a5a, 0, 0, 0, 0.02f),
        BLKB(0.06f, 0.07f, 0.06f,  0.09f, 0.27f, 0, 0xff9a5a, 0, 0, 0, 0.02f),
    };
    gDropModels[DROP_DMG] = model_from_blocks(dmg, 6, &rng, false, gScene.litShader);

    BlockSpec ammo[4] = {
        BLKB(0.52f, 0.3f, 0.28f, 0, 0, 0, 0x4f5634, 0, 0, 0, 0.04f),
        BLKB(0.54f, 0.06f, 0.3f, 0, 0.17f, 0, 0x3d4228, 0, 0, 0, 0.02f),
        BLKB(0.24f, 0.04f, 0.06f, 0, 0.215f, 0, 0x2a2c24, 0, 0, 0, 0.01f),
        BLKB(0.3f, 0.06f, 0.006f, 0, 0, 0.143f, 0xd8b84a, 0, 0, 0, 0.002f),
    };
    gDropModels[DROP_AMMO] = model_from_blocks(ammo, 4, &rng, false, gScene.litShader);
}

void drops_clear(void) {
    memset(gDrops, 0, sizeof(gDrops));
}

void drops_cleanup(void) {
    for (int i = 0; i < 3; i++) UnloadModel(gDropModels[i]);
}

void drops_maybe_spawn(Vector3 pos) {
    gState.sinceDrop++;
    if (rnd01() > DROP_CHANCE + gState.dropBonus && gState.sinceDrop < DROP_PITY) return;
    gState.sinceDrop = 0;

    bool allowMed = gState.hp < gState.maxHp;
    float total = (allowMed ? DROP_WEIGHTS[0] : 0.0f) + DROP_WEIGHTS[1] + DROP_WEIGHTS[2];
    float r = rnd01() * total;
    DropKind pick = DROP_AMMO;
    for (int i = allowMed ? 0 : 1; i < 3; i++) {
        r -= DROP_WEIGHTS[i];
        if (r < 0.0f) { pick = (DropKind)i; break; }
    }

    for (int i = 0; i < MAX_DROPS; i++) {
        if (!gDrops[i].active) {
            gDrops[i] = (DropItem){
                .active = true,
                .kind = pick,
                .pos = { pos.x, 0.7f, pos.z },
                .rotY = 0.0f,
                .life = DROP_LIFE,
                .t = rndf(0.0f, 6.0f),
                .visible = true
            };
            break;
        }
    }
}

static void collect_drop(DropKind kind) {
    if (kind == DROP_MED) {
        if (gState.hp < gState.maxHp) gState.hp++;
    } else if (kind == DROP_DMG) {
        gState.pDamage = 10.0f;
    } else if (kind == DROP_AMMO) {
        gState.pAmmo = 8.0f;
        gState.reloading = false;
        gState.mag = gState.magSize;
    }
    if (gCollectCb) gCollectCb(kind, DROP_NAMES[kind], hex_color(DROP_COLORS[kind], 255));
}

void drops_update(float dt) {
    float lim = WORLD_HALF - 0.9f;
    for (int i = 0; i < MAX_DROPS; i++) {
        DropItem *d = &gDrops[i];
        if (!d->active) continue;
        d->life -= dt;
        d->t += dt;

        float dx = gPlayer.position.x - d->pos.x;
        float dz = gPlayer.position.z - d->pos.z;
        float dist = hypotf(dx, dz);
        if (dist < 0.9f) {
            collect_drop(d->kind);
            d->active = false;
            continue;
        }
        if (dist < 2.5f) {
            d->pos.x += (dx / dist) * 10.0f * dt;
            d->pos.z += (dz / dist) * 10.0f * dt;
        } else {
            float cx = clampf(d->pos.x, -lim, lim);
            float cz = clampf(d->pos.z, -lim, lim);
            float ox = cx - d->pos.x, oz = cz - d->pos.z;
            float od = hypotf(ox, oz);
            if (od > 0.01f) {
                float st = fminf(od, 6.0f * dt);
                d->pos.x += (ox / od) * st;
                d->pos.z += (oz / od) * st;
            }
        }
        d->pos.y = 0.7f + sinf(d->t * 3.0f) * 0.15f;
        d->rotY += dt * 2.0f;
        d->visible = d->life > 3.0f || (((int)(d->t * 8.0f)) % 2 == 0);
        if (d->life <= 0.0f) d->active = false;
    }
}

void drops_draw(void) {
    for (int i = 0; i < MAX_DROPS; i++) {
        DropItem *d = &gDrops[i];
        if (!d->active || !d->visible) continue;

        float em[3] = { 0.11f, 0.11f, 0.11f };
        SetShaderValue(gScene.litShader, gScene.locEmissive, em, SHADER_UNIFORM_VEC3);
        Matrix m = MatrixMultiply(
            MatrixScale(1.25f, 1.25f, 1.25f),
            make_transform_yxz(d->pos.x, d->pos.y, d->pos.z, 0.0f, d->rotY, 0.0f)
        );
        DrawMesh(gDropModels[d->kind].meshes[0], gDropModels[d->kind].materials[0], m);
        float zero[3] = { 0, 0, 0 };
        SetShaderValue(gScene.litShader, gScene.locEmissive, zero, SHADER_UNIFORM_VEC3);

        rlDrawRenderBatchActive();
        BeginBlendMode(BLEND_ADDITIVE);
        Color gc = hex_color(DROP_COLORS[d->kind], 90);
        DrawSphere(d->pos, 0.55f, gc);
        EndBlendMode();
    }
}
