#include "openings.h"
#include "core/math_util.h"
#include "core/scene.h"
#include "effects/particles.h"
#include "audio/sfx.h"
#include "raymath.h"
#include <math.h>
#include <float.h>
#include <stddef.h>

Opening gOpenings[MAX_OPENINGS];
int gOpeningCount = 0;

static const uint32_t OP_POST = 0x3a2b1d;
static const uint32_t OP_WEATHERED = 0x6d6352;
static const float OP_LOG_D = 0.62f;

Opening *make_opening(char axis, float fixed, float u0, float u1, OpeningKind kind, float wallT) {
    if (gOpeningCount >= MAX_OPENINGS) return NULL;
    Opening *o = &gOpenings[gOpeningCount++];
    *o = (Opening){0};
    o->kind = kind;
    o->axis = axis;
    o->u0 = u0;
    o->u1 = u1;
    o->w = u1 - u0;
    o->climberIdx = -1;
    o->fallT = -1.0f;

    float c = (u0 + u1) * 0.5f;
    float s = (fixed >= 0.0f) ? 1.0f : -1.0f;
    float ht = wallT * 0.5f;

    if (axis == 'x') {
        o->x = c; o->z = fixed;
        o->ox = c; o->oz = fixed + s * OPENING_INSET;
        o->ix = c; o->iz = fixed - s * OPENING_INSET;
        o->fx = c; o->fz = fixed + s * 0.85f;
        o->lx = c; o->lz = fixed - s * 0.95f;
        o->sill = 0.85f; o->head = 1.63f;
        o->rotY = (s > 0.0f) ? 0.0f : PI;
        o->box = (Box2D){ u0, u1, fixed - ht, fixed + ht };
    } else {
        o->x = fixed; o->z = c;
        o->ox = fixed + s * OPENING_INSET; o->oz = c;
        o->ix = fixed - s * OPENING_INSET; o->iz = c;
        o->fx = fixed + s * 0.85f; o->fz = c;
        o->lx = fixed - s * 0.95f; o->lz = c;
        o->sill = 1.05f; o->head = 1.83f;
        o->rotY = s * PI * 0.5f;
        o->box = (Box2D){ fixed - ht, fixed + ht, u0, u1 };
    }

    o->maxHp = (kind == OPENING_DOOR) ? DOOR_HP : WINDOW_HP;
    o->hp = o->maxHp;
    return o;
}

void fit_opening(Opening *o, BlockBatch *wallB, SeededRng *wrng, Shader shader) {
    Matrix place = make_transform_yxz(o->x, 0.0f, o->z, 0.0f, o->rotY, 0.0f);
    float hw = o->w * 0.5f;
    SeededRng frng = rng_seed((uint32_t)(roundf(o->x * 7.0f + o->z * 13.0f) + 99));

    if (o->kind == OPENING_DOOR) {
        BlockSpec frame[4] = {
            BLKB(JAMB, DOOR_TOP + 0.1f, OP_LOG_D + 0.1f, -hw + JAMB * 0.5f, (DOOR_TOP + 0.1f) * 0.5f, 0, OP_POST, 0, 0, 0, 0.03f),
            BLKB(JAMB, DOOR_TOP + 0.1f, OP_LOG_D + 0.1f,  hw - JAMB * 0.5f, (DOOR_TOP + 0.1f) * 0.5f, 0, OP_POST, 0, 0, 0, 0.03f),
            BLKB(o->w + 0.1f, 0.14f, OP_LOG_D + 0.12f, 0, DOOR_TOP + 0.02f, 0, OP_POST, 0, 0, 0, 0.03f),
            BLKB(o->w - 0.1f, 0.05f, OP_LOG_D, 0, 0.0f, 0, OP_POST, 0, 0, 0, 0.02f),
        };
        batch_add(wallB, frame, 4, wrng, &place, false);

        float lw = o->w - 2.0f * JAMB - 0.02f;
        float lh = 1.92f;
        int n = 4;
        float pw = lw / (float)n;
        BlockSpec blocks[16];
        int bc = 0;
        for (int i = 0; i < n; i++) {
            blocks[bc++] = BLKB(pw - 0.012f, lh, 0.06f, pw * ((float)i + 0.5f), lh * 0.5f, 0,
                                (i % 2) ? OP_WEATHERED : 0x5f5646, 0, 0, 0, 0.012f);
        }
        blocks[bc++] = BLKB(lw - 0.04f, 0.14f, 0.04f, lw * 0.5f, 0.35f, -0.05f, OP_POST, 0, 0, 0, 0.012f);
        blocks[bc++] = BLKB(lw - 0.04f, 0.14f, 0.04f, lw * 0.5f, lh - 0.35f, -0.05f, OP_POST, 0, 0, 0, 0.012f);
        float bl = hypotf(lw - 0.1f, lh - 0.7f);
        blocks[bc++] = BLKB(0.12f, bl, 0.04f, lw * 0.5f, lh * 0.5f, -0.05f, OP_POST, 0, 0, -atan2f(lw - 0.1f, lh - 0.7f), 0.012f);
        blocks[bc++] = BLKB(0.05f, 0.05f, 0.08f, lw - 0.15f, 1.0f, 0.06f, 0x2a2a2a, 0, 0, 0, 0.01f);
        blocks[bc++] = BLKB(0.32f, 0.05f, 0.015f, 0.16f, 0.35f, 0.035f, 0x2a2a2a, 0, 0, 0, 0.005f);
        blocks[bc++] = BLKB(0.32f, 0.05f, 0.015f, 0.16f, lh - 0.35f, 0.035f, 0x2a2a2a, 0, 0, 0, 0.005f);

        o->leafModel = model_from_blocks(blocks, bc, &frng, false, shader);
        o->fallPos = (Vector3){ 0.0f, 0.0f, -0.02f };
    } else {
        float h = o->head - o->sill;
        float mid = (o->sill + o->head) * 0.5f;
        float iw = o->w - 0.2f;
        BlockSpec frame[4] = {
            BLKB(0.1f, h + 0.1f, OP_LOG_D + 0.06f, -hw + 0.05f, mid, 0, OP_POST, 0, 0, 0, 0.02f),
            BLKB(0.1f, h + 0.1f, OP_LOG_D + 0.06f,  hw - 0.05f, mid, 0, OP_POST, 0, 0, 0, 0.02f),
            BLKB(o->w + 0.12f, 0.08f, OP_LOG_D + 0.08f, 0, o->head, 0, OP_POST, 0, 0, 0, 0.02f),
            BLKB(o->w + 0.3f, 0.07f, OP_LOG_D + 0.24f, 0, o->sill, 0.05f, OP_WEATHERED, 0, 0, 0, 0.02f),
        };
        batch_add(wallB, frame, 4, wrng, &place, false);

        BlockSpec glassBlk = BLKB(iw, h - 0.08f, 0.02f, 0, 0, 0, 0x9fb6c9, 0, 0, 0, 0.0f);
        o->glassModel = model_from_blocks(&glassBlk, 1, &frng, true, shader);

        BlockSpec bars[2] = {
            BLK(0.05f, h - 0.08f, 0.05f, 0, 0, 0, OP_POST),
            BLK(iw, 0.05f, 0.05f, 0, 0, 0, OP_POST)
        };
        o->barsModel = model_from_blocks(bars, 2, &frng, false, shader);

        BlockSpec teeth[12];
        for (int i = 0; i < 9; i++) {
            int side = i % 3;
            float t = rng_next(&frng) - 0.5f;
            float len = 0.12f + rng_next(&frng) * 0.2f;
            float px, py;
            if (side == 0) { px = t * iw; py = -(h - 0.1f) * 0.5f + 0.05f; }
            else if (side == 1) { px = -iw * 0.5f + 0.05f; py = t * (h - 0.1f); }
            else { px = iw * 0.5f - 0.05f; py = t * (h - 0.1f); }
            teeth[i] = BLKB(0.03f + rng_next(&frng) * 0.06f, len, 0.015f, px, py, 0, 0xd8e4ee,
                            0, 0, (rng_next(&frng) - 0.5f) * 1.6f, 0.0f);
        }
        for (int i = 0; i < 3; i++) {
            teeth[9 + i] = BLKB(0.04f + rng_next(&frng) * 0.05f, 0.1f + rng_next(&frng) * 0.12f, 0.015f,
                                (rng_next(&frng) - 0.5f) * iw, (h - 0.1f) * 0.5f - 0.04f, 0, 0xd8e4ee,
                                0, 0, PI + (rng_next(&frng) - 0.5f) * 1.2f, 0.0f);
        }
        o->shardsModel = model_from_blocks(teeth, 12, &frng, true, shader);
    }
    reset_openings();
}

void reset_openings(void) {
    for (int i = 0; i < gOpeningCount; i++) {
        Opening *o = &gOpenings[i];
        o->hp = o->maxHp;
        o->climberIdx = -1;
        o->jolt = 0.0f;
        o->fallT = -1.0f;
        o->leafRotY = 0.0f;
        o->fallRotX = 0.0f;
        o->fallPos = (Vector3){ 0.0f, 0.0f, -0.02f };
        o->glassVisible = true;
        o->glassCracked = false;
        o->barsVisible = true;
        o->shardsVisible = false;
        o->paneZ = 0.0f;
    }
}

void update_openings(float dt) {
    for (int i = 0; i < gOpeningCount; i++) {
        Opening *o = &gOpenings[i];
        o->jolt *= expf(-9.0f * dt);
        if (o->kind == OPENING_WINDOW) {
            o->paneZ = -0.03f * o->jolt;
            continue;
        }
        if (o->fallT >= 0.0f) {
            o->fallT += dt;
            float k = fminf(1.0f, o->fallT / 0.5f);
            float slap = k >= 1.0f ? 0.05f * expf(-(o->fallT - 0.5f) * 10.0f) * sinf((o->fallT - 0.5f) * 30.0f) : 0.0f;
            o->fallRotX = -(PI * 0.5f - 0.02f) * k * k + slap;
            o->fallPos = (Vector3){ 0.0f, 0.08f * k, -0.02f - 0.35f * k };
            continue;
        }
        if (o->hp <= 0) continue;
        o->leafRotY = o->jolt * 0.07f;
    }
}

void draw_openings(Shader shader) {
    (void)shader;
    for (int i = 0; i < gOpeningCount; i++) {
        Opening *o = &gOpenings[i];
        Matrix root = make_transform_yxz(o->x, 0.0f, o->z, 0.0f, o->rotY, 0.0f);
        if (o->kind == OPENING_DOOR) {
            float hw = o->w * 0.5f;
            Matrix fallM = make_transform_yxz(o->fallPos.x, o->fallPos.y, o->fallPos.z, o->fallRotX, 0.0f, 0.0f);
            Matrix leafM = make_transform_yxz(-hw + JAMB + 0.01f, 0.02f, 0.0f, 0.0f, o->leafRotY, 0.0f);
            Matrix m = MatrixMultiply(MatrixMultiply(leafM, fallM), root);
            o->leafModel.transform = m;
            DrawModel(o->leafModel, (Vector3){0}, 1.0f, WHITE);
        } else {
            float mid = (o->sill + o->head) * 0.5f;
            Matrix paneM = make_transform_yxz(0.0f, mid, o->paneZ, 0.0f, 0.0f, 0.0f);
            Matrix m = MatrixMultiply(paneM, root);
            if (o->barsVisible) {
                o->barsModel.transform = m;
                DrawModel(o->barsModel, (Vector3){0}, 1.0f, WHITE);
            }
            if (o->shardsVisible) {
                o->shardsModel.transform = m;
                DrawModel(o->shardsModel, (Vector3){0}, 1.0f, (Color){ 200, 220, 235, 110 });
            }
            if (o->glassVisible) {
                o->glassModel.transform = m;
                Color gc = o->glassCracked ? (Color){ 216, 228, 238, 145 } : (Color){ 159, 182, 201, 80 };
                DrawModel(o->glassModel, (Vector3){0}, 1.0f, gc);
            }
        }
    }
}

void unload_openings(void) {
    for (int i = 0; i < gOpeningCount; i++) {
        Opening *o = &gOpenings[i];
        if (o->kind == OPENING_DOOR) {
            UnloadModel(o->leafModel);
        } else {
            UnloadModel(o->glassModel);
            UnloadModel(o->barsModel);
            UnloadModel(o->shardsModel);
        }
    }
    gOpeningCount = 0;
}

Opening *shut_at(float x, float z) {
    for (int i = 0; i < gOpeningCount; i++) {
        Opening *o = &gOpenings[i];
        if (o->hp > 0 && x >= o->box.minX && x <= o->box.maxX && z >= o->box.minZ && z <= o->box.maxZ) {
            return o;
        }
    }
    return NULL;
}

Opening *nearest_opening(Vector3 pos, const Opening *skip) {
    Opening *best = NULL;
    float bd = INFINITY;
    for (int i = 0; i < gOpeningCount; i++) {
        Opening *o = &gOpenings[i];
        if (o == skip) continue;
        float dx = o->x - pos.x, dz = o->z - pos.z;
        float d = dx * dx + dz * dz;
        if (d < bd) { bd = d; best = o; }
    }
    return best;
}

void hit_opening(Opening *o) {
    if (!o || o->hp <= 0) return;
    o->hp--;
    o->jolt = 1.0f;
    if (o->kind == OPENING_DOOR) {
        sfx_bash((Vector3){ o->x, 0.0f, o->z }, 1.0f);
        Vector3 into = Vector3Normalize((Vector3){ o->ix - o->x, 0, o->iz - o->z });
        spawn_opening_burst((Vector3){ o->x, 0, o->z }, into, o->w, true, o->sill, o->head,
                            2, DEBRIS_WOOD, 0.12f, 2.0f);
    } else if (o->hp > 0) {
        o->glassCracked = true;
        sfx_crack((Vector3){ o->x, 0.0f, o->z });
    }
    if (o->hp <= 0) break_opening(o);
}

void break_opening(Opening *o) {
    if (!o) return;
    o->hp = 0;
    Vector3 into = Vector3Normalize((Vector3){ o->ix - o->x, 0, o->iz - o->z });
    if (o->kind == OPENING_DOOR) {
        o->fallT = 0.0f;
        o->leafRotY = 0.0f;
        sfx_door_break((Vector3){ o->x, 0.0f, o->z });
        spawn_opening_burst((Vector3){ o->x, 0, o->z }, into, o->w, true, o->sill, o->head,
                            9, DEBRIS_WOOD, 0.16f, 3.5f);
    } else {
        o->glassVisible = false;
        o->barsVisible = false;
        o->shardsVisible = true;
        sfx_shatter((Vector3){ o->x, 0.0f, o->z });
        spawn_opening_burst((Vector3){ o->x, 0, o->z }, into, o->w, false, o->sill, o->head,
                            16, DEBRIS_GLASS, 0.09f, 4.0f);
    }
}
