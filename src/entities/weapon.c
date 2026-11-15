#include "weapon.h"
#include "player.h"
#include "core/state.h"
#include "core/math_util.h"
#include "world/world.h"
#include "world/openings.h"
#include "effects/particles.h"
#include "rlgl.h"
#include <math.h>
#include <string.h>

Bullet gBullets[MAX_BULLETS];

void weapon_clear(void) {
    memset(gBullets, 0, sizeof(gBullets));
}

void weapon_start_reload(void) {
    if (!gState.running || gState.reloading || gState.mag == gState.magSize) return;
    gState.reloading = true;
    gState.reloadT = 0.0f;
}

void weapon_finish_reload(void) {
    gState.reloading = false;
    gState.mag = gState.magSize;
}

void weapon_fire(Vector3 aimPoint) {
    if (gState.fireCd > 0.0f || gState.reloading || gState.mag <= 0 || !gState.running) return;
    if (gState.pAmmo <= 0.0f) {
        gState.mag--;
        if (gState.mag <= 0) weapon_start_reload();
    }
    gState.shots++;
    gState.fireCd = gState.fireRate;

    Vector3 aim = { aimPoint.x - gPlayer.position.x, 0.0f, aimPoint.z - gPlayer.position.z };
    if (Vector3LengthSqr(aim) < 1e-4f) return;
    aim = Vector3Normalize(aim);

    Vector3 mp = gPlayer.muzzleWorldPos;
    int count = 1 + gState.spread;
    bool hot = gState.pDamage > 0.0f;

    for (int k = 0; k < count; k++) {
        float a = ((float)k - (float)(count - 1) * 0.5f) * 0.13f;
        float c = cosf(a), s = sinf(a);
        Vector3 dir = { aim.x * c - aim.z * s, 0.0f, aim.x * s + aim.z * c };

        for (int i = 0; i < MAX_BULLETS; i++) {
            if (!gBullets[i].active) {
                gBullets[i] = (Bullet){
                    .active = true,
                    .pos = mp,
                    .dir = dir,
                    .life = 0.9f,
                    .pierce = gState.pierce,
                    .hot = hot,
                    .hitCount = 0,
                };
                break;
            }
        }
    }

    player_muzzle_flash(hot);

    // Eject brass casing & muzzle smoke
    Vector3 right = Vector3Normalize((Vector3){ -gPlayer.mGun.m0, -gPlayer.mGun.m1, -gPlayer.mGun.m2 });
    Vector3 back  = Vector3Normalize((Vector3){ -gPlayer.mGun.m8, -gPlayer.mGun.m9, -gPlayer.mGun.m10 });
    Vector3 ejectPos = Vector3Add(
        (Vector3){ gPlayer.mGun.m12, gPlayer.mGun.m13, gPlayer.mGun.m14 },
        Vector3Add(Vector3Scale(right, 0.08f), Vector3Scale(back, -0.48f))
    );
    Vector3 casVel = {
        right.x * rndf(2.4f, 3.6f) + back.x * 0.6f,
        rndf(3.0f, 4.0f),
        right.z * rndf(2.4f, 3.6f) + back.z * 0.6f
    };
    spawn_debris(DEBRIS_CASING, ejectPos, casVel,
                 (Vector3){ 0, gPlayer.rotY, 0 },
                 (Vector3){ rndf(-10, 10), rndf(-10, 10), rndf(-10, 10) },
                 (Vector3){ 1, 1, 1 }, 2.5f, 0.025f);

    Vector3 fwd = Vector3Negate(back);
    spawn_debris(DEBRIS_SMOKE, mp, Vector3Scale(fwd, 1.6f),
                 (Vector3){ rndf(0, 3), rndf(0, 3), 0 }, (Vector3){0},
                 (Vector3){ rndf(0.14f, 0.20f), 0, 0 }, 0.55f, 0.0f);

    gState.shake = fmaxf(gState.shake, 0.12f);
}

void weapon_update_bullets(float dt) {
    for (int i = 0; i < MAX_BULLETS; i++) {
        Bullet *b = &gBullets[i];
        if (!b->active) continue;
        float bpx = b->pos.x, bpz = b->pos.z;
        b->pos = Vector3Add(b->pos, Vector3Scale(b->dir, 38.0f * dt));
        b->life -= dt;

        bool hit = false;
        float dist = hypotf(b->pos.x - bpx, b->pos.z - bpz);
        int steps = (int)ceilf(dist / (WALL_T * 0.5f));
        if (steps < 1) steps = 1;

        for (int s = 1; s <= steps && !hit; s++) {
            float t = (float)s / (float)steps;
            float sx = bpx + (b->pos.x - bpx) * t;
            float sz = bpz + (b->pos.z - bpz) * t;
            for (int w = 0; w < gWorld.wallBoxCount; w++) {
                Box2D wb = gWorld.wallBoxes[w];
                if (sx >= wb.minX && sx <= wb.maxX && sz >= wb.minZ && sz <= wb.maxZ) {
                    hit = true;
                    break;
                }
            }
            if (!hit) {
                Opening *o = shut_at(sx, sz);
                if (o) {
                    if (o->kind == OPENING_DOOR) {
                        hit = true;
                        o->jolt = 0.4f;
                        Vector3 into = Vector3Normalize((Vector3){ o->ix - o->x, 0, o->iz - o->z });
                        spawn_opening_burst((Vector3){ o->x, 0, o->z }, into, o->w, true, o->sill, o->head,
                                            1, DEBRIS_WOOD, 0.08f, 1.5f);
                    } else {
                        break_opening(o);
                        Vector3 into = Vector3Normalize((Vector3){ o->ix - o->x, 0, o->iz - o->z });
                        spawn_opening_burst((Vector3){ o->x, 0, o->z }, into, o->w, false, o->sill, o->head,
                                            16, DEBRIS_GLASS, 0.09f, 4.0f);
                    }
                }
            }
        }

        if (hit || b->life <= 0.0f) b->active = false;
    }
}

void weapon_draw_bullets(void) {
    rlDrawRenderBatchActive();
    BeginBlendMode(BLEND_ADDITIVE);
    for (int i = 0; i < MAX_BULLETS; i++) {
        Bullet *b = &gBullets[i];
        if (!b->active) continue;
        Vector3 tail = Vector3Subtract(b->pos, Vector3Scale(b->dir, 0.9f));
        Vector3 tailGlow = Vector3Subtract(b->pos, Vector3Scale(b->dir, 1.05f));
        Color core = hex_color(0xfff4d6, 255);
        Color glow = b->hot ? hex_color(0xff5a1e, 155) : hex_color(0xffb04a, 115);
        DrawCylinderEx(tailGlow, b->pos, 0.08f, 0.08f, 4, glow);
        DrawCylinderEx(tail, b->pos, 0.025f, 0.025f, 4, core);
    }
    EndBlendMode();
}
