#include "solids.h"
#include <math.h>
#include <float.h>

SolidBox gSolids[MAX_SOLIDS];
int gSolidCount = 0;
NavNode gRoundNodes[MAX_ROUND_NODES];
int gRoundNodeCount = 0;
static float gNodeLinks[MAX_ROUND_NODES][MAX_ROUND_NODES];

void solids_clear(void) {
    gSolidCount = 0;
    gRoundNodeCount = 0;
}

void add_solid(float x, float z, float ry, float hx, float hz) {
    if (gSolidCount >= MAX_SOLIDS) return;
    gSolids[gSolidCount++] = (SolidBox){
        .x = x, .z = z, .hx = hx, .hz = hz,
        .c = cosf(ry), .s = sinf(ry)
    };
}

static void to_local(const SolidBox *b, float x, float z, float *lx, float *lz) {
    float dx = x - b->x, dz = z - b->z;
    *lx = dx * b->c - dz * b->s;
    *lz = dx * b->s + dz * b->c;
}

void push_out_of_solids(Vector3 *pos) {
    for (int i = 0; i < gSolidCount; i++) {
        const SolidBox *b = &gSolids[i];
        float lx, lz;
        to_local(b, pos->x, pos->z, &lx, &lz);
        float ex = b->hx + Z_RADIUS;
        float ez = b->hz + Z_RADIUS;
        float px = ex - fabsf(lx);
        float pz = ez - fabsf(lz);
        if (px <= 0.0f || pz <= 0.0f) continue;

        float nx = px < pz ? (lx >= 0.0f ? 1.0f : -1.0f) * (ex + 0.01f) : lx;
        float nz = px < pz ? lz : (lz >= 0.0f ? 1.0f : -1.0f) * (ez + 0.01f);
        pos->x = b->x + nx * b->c + nz * b->s;
        pos->z = b->z - nx * b->s + nz * b->c;
    }
}

bool seg_hits_box(float ax, float az, float bx, float bz, float hx, float hz) {
    float t0 = 0.0f, t1 = 1.0f;
    float dx = bx - ax, dz = bz - az;
    if (fabsf(dx) < 1e-6f) {
        if (fabsf(ax) >= hx) return false;
    } else {
        float ta = (-hx - ax) / dx;
        float tb = (hx - ax) / dx;
        float lo = ta < tb ? ta : tb;
        float hi = ta > tb ? ta : tb;
        if (lo > t0) t0 = lo;
        if (hi < t1) t1 = hi;
    }
    if (fabsf(dz) < 1e-6f) {
        if (fabsf(az) >= hz) return false;
    } else {
        float ta = (-hz - az) / dz;
        float tb = (hz - az) / dz;
        float lo = ta < tb ? ta : tb;
        float hi = ta > tb ? ta : tb;
        if (lo > t0) t0 = lo;
        if (hi < t1) t1 = hi;
    }
    return t0 < t1;
}

#define SEE_SLOP 0.05f

bool clear_path(float ax, float az, float bx, float bz) {
    for (int i = 0; i < gSolidCount; i++) {
        const SolidBox *b = &gSolids[i];
        float px, pz, qx, qz;
        to_local(b, ax, az, &px, &pz);
        to_local(b, bx, bz, &qx, &qz);
        if (seg_hits_box(px, pz, qx, qz, b->hx + Z_RADIUS - SEE_SLOP, b->hz + Z_RADIUS - SEE_SLOP)) {
            return false;
        }
    }
    return true;
}

void init_solids_nav(void) {
    gRoundNodeCount = 0;
    const float corners[4][2] = { {1, 1}, {1, -1}, {-1, -1}, {-1, 1} };
    for (int i = 0; i < gSolidCount; i++) {
        const SolidBox *b = &gSolids[i];
        for (int c = 0; c < 4; c++) {
            float kx = corners[c][0] * (b->hx + Z_RADIUS + ROUND_PAD);
            float kz = corners[c][1] * (b->hz + Z_RADIUS + ROUND_PAD);
            gRoundNodes[gRoundNodeCount++] = (NavNode){
                .x = b->x + kx * b->c + kz * b->s,
                .z = b->z - kx * b->s + kz * b->c
            };
        }
    }
    for (int i = 0; i < gRoundNodeCount; i++) {
        for (int j = 0; j < gRoundNodeCount; j++) {
            if (i != j && clear_path(gRoundNodes[i].x, gRoundNodes[i].z, gRoundNodes[j].x, gRoundNodes[j].z)) {
                gNodeLinks[i][j] = hypotf(gRoundNodes[i].x - gRoundNodes[j].x, gRoundNodes[i].z - gRoundNodes[j].z);
            } else {
                gNodeLinks[i][j] = INFINITY;
            }
        }
    }
}

Vector2 route_around(Vector3 pos, float tx, float tz, int *via) {
    Vector3 from = pos;
    push_out_of_solids(&from);
    float x = from.x, fz = from.z;

    if (clear_path(x, fz, tx, tz)) {
        if (via) *via = -1;
        return (Vector2){ tx, tz };
    }

    int n = gRoundNodeCount;
    float dist[MAX_ROUND_NODES];
    bool done[MAX_ROUND_NODES];
    for (int i = 0; i < n; i++) {
        NavNode k = gRoundNodes[i];
        dist[i] = clear_path(k.x, k.z, tx, tz) ? hypotf(tx - k.x, tz - k.z) : INFINITY;
        done[i] = false;
    }

    for (int r = 0; r < n; r++) {
        int u = -1;
        for (int i = 0; i < n; i++) {
            if (!done[i] && (u < 0 || dist[i] < dist[u])) u = i;
        }
        if (u < 0 || isinf(dist[u])) break;
        done[u] = true;
        for (int i = 0; i < n; i++) {
            float alt = dist[u] + gNodeLinks[u][i];
            if (alt < dist[i]) dist[i] = alt;
        }
    }

    int best = -1, near = -1;
    float bestCost = INFINITY, nearD = INFINITY;
    int curVia = via ? *via : -1;
    for (int i = 0; i < n; i++) {
        NavNode k = gRoundNodes[i];
        float d = hypotf(k.x - x, k.z - fz);
        if (d < 0.45f || !clear_path(x, fz, k.x, k.z)) continue;
        if (d < nearD) { nearD = d; near = i; }
        float cost = d + dist[i] - (curVia == i ? 1.0f : 0.0f);
        if (cost < bestCost) { bestCost = cost; best = i; }
    }

    if (best < 0) best = near;
    if (best < 0) return (Vector2){ tx, tz };
    if (via) *via = best;
    return (Vector2){ gRoundNodes[best].x, gRoundNodes[best].z };
}

bool is_inside_cabin(Vector3 pos, float half) {
    return fabsf(pos.x) < half - 0.3f && fabsf(pos.z) < half - 0.3f;
}
