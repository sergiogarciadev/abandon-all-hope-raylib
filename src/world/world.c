#include "world.h"
#include "solids.h"
#include "core/math_util.h"
#include "core/blocks.h"
#include "core/scene.h"
#include "rlgl.h"
#include <math.h>
#include <stdbool.h>
#include <stddef.h>

GameWorld gWorld = {0};

static const uint32_t LOG_TONES[4] = { 0x5a4430, 0x4d3926, 0x664e36, 0x45331f };
static const uint32_t END_GRAIN = 0x8d7150;
static const uint32_t CHINK = 0x9a9486;
static const uint32_t POST = 0x3a2b1d;
static const uint32_t WEATHERED = 0x6d6352;
static const uint32_t STONE_TONES[3] = { 0x3d3e40, 0x46464a, 0x353638 };

static const float LOG_STEP = 0.4f;
static const float LOG_H = 0.42f;
static const float LOG_D = 0.62f;
static const float OVERHANG = 0.42f;

static Texture2D make_ground_texture(void) {
    Image img = GenImageColor(128, 128, hex_color(0x1b1e15, 255));
    SeededRng rng = rng_seed(5);
    const uint32_t tones[6] = { 0x232b19, 0x161812, 0x2a2418, 0x1e2716, 0x121410, 0x2c3320 };
    for (int i = 0; i < 2600; i++) {
        Color c = hex_color(tones[(int)(rng_next(&rng) * 6.0f) % 6], 255);
        int s = rng_next(&rng) < 0.85f ? 2 : 4;
        int x = ((int)(rng_next(&rng) * 64.0f)) * 2;
        int y = ((int)(rng_next(&rng) * 64.0f)) * 2;
        ImageDrawRectangle(&img, x, y, s, s, c);
    }
    Texture2D tex = LoadTextureFromImage(img);
    UnloadImage(img);
    SetTextureWrap(tex, TEXTURE_WRAP_REPEAT);
    SetTextureFilter(tex, TEXTURE_FILTER_POINT);
    return tex;
}

static Texture2D make_moon_texture(void) {
    Image img = GenImageColor(256, 256, (Color){ 0, 0, 0, 0 });
    ImageDrawCircle(&img, 128, 128, 120, (Color){ 160, 180, 220, 18 });
    ImageDrawCircle(&img, 128, 128, 70, (Color){ 180, 195, 230, 42 });
    ImageDrawCircle(&img, 128, 128, 38, (Color){ 200, 214, 240, 110 });
    ImageDrawCircle(&img, 128, 128, 22, hex_color(0xe4e9f2, 255));
    ImageDrawCircle(&img, 121, 122, 7, (Color){ 150, 160, 180, 90 });
    ImageDrawCircle(&img, 134, 131, 5, (Color){ 150, 160, 180, 90 });
    ImageDrawCircle(&img, 126, 137, 4, (Color){ 150, 160, 180, 90 });
    Texture2D tex = LoadTextureFromImage(img);
    UnloadImage(img);
    return tex;
}

static BlockSpec wall_block(char axis, float fixed, float u0, float u1, float y, float h, float d,
                            uint32_t color, float bevel, float n, float tilt) {
    float len = fabsf(u1 - u0);
    float c = (u0 + u1) * 0.5f;
    if (axis == 'x') {
        return BLKB(len, h, d, c, y, fixed + n, color, 0, 0, tilt, bevel);
    } else {
        return BLKB(d, h, len, fixed + n, y, c, color, -tilt, 0, 0, bevel);
    }
}

static bool keep_always(float y, const Opening *o) { (void)y; (void)o; return true; }
static bool keep_door(float y, const Opening *o) { (void)o; return y - LOG_H * 0.5f >= DOOR_TOP - 0.01f; }
static bool keep_window(float y, const Opening *o) {
    return y + LOG_H * 0.5f <= o->sill + 0.01f || y - LOG_H * 0.5f >= o->head - 0.01f;
}

static void log_run(BlockBatch *wallB, SeededRng *wrng, char axis, float fixed, float a, float b,
                    float layer0, bool (*keep)(float, const Opening *), const Opening *o,
                    bool cutA, bool cutB, bool footing) {
    for (int k = 0; k < 6; k++) {
        float y = layer0 + (float)k * LOG_STEP;
        if (!keep(y, o)) continue;
        float u0 = cutA ? a : a - OVERHANG;
        float u1 = cutB ? b : b + OVERHANG;
        BlockSpec logB = wall_block(axis, fixed, u0, u1, y, LOG_H, LOG_D,
                                    LOG_TONES[(int)(rng_next(wrng) * 4.0f) % 4], 0.19f, 0, 0);
        batch_add(wallB, &logB, 1, wrng, NULL, false);
        if (k < 5 && keep(y + LOG_STEP, o)) {
            BlockSpec chinkB = wall_block(axis, fixed, a, b, y + LOG_STEP * 0.5f, 0.08f, LOG_D * 0.72f, CHINK, 0.02f, 0, 0);
            batch_add(wallB, &chinkB, 1, wrng, NULL, false);
        }
        if (!cutA) {
            BlockSpec eg = wall_block(axis, fixed, u0 - 0.012f, u0 + 0.012f, y, LOG_H * 0.8f, LOG_D * 0.8f, END_GRAIN, 0.1f, 0, 0);
            batch_add(wallB, &eg, 1, wrng, NULL, false);
        }
        if (!cutB) {
            BlockSpec eg = wall_block(axis, fixed, u1 - 0.012f, u1 + 0.012f, y, LOG_H * 0.8f, LOG_D * 0.8f, END_GRAIN, 0.1f, 0, 0);
            batch_add(wallB, &eg, 1, wrng, NULL, false);
        }
    }
    if (!footing) return;
    if (axis == 'z') {
        BlockSpec sill = wall_block(axis, fixed, a, b, 0.13f, 0.24f, LOG_D * 0.9f, LOG_TONES[1], 0.06f, 0, 0);
        batch_add(wallB, &sill, 1, wrng, NULL, false);
    }
    float endU = b + (cutB ? 0.0f : 0.3f);
    for (float u = a - (cutA ? 0.0f : 0.3f); u < endU;) {
        float len = 0.45f + rng_next(wrng) * 0.6f;
        float uNext = fminf(u + len, endU) - 0.04f;
        BlockSpec st = wall_block(axis, fixed, u, uNext, 0.02f, 0.18f, LOG_D + 0.16f,
                                  STONE_TONES[(int)(rng_next(wrng) * 3.0f) % 3], 0.05f, 0, 0);
        batch_add(wallB, &st, 1, wrng, NULL, false);
        u += len;
    }
}

typedef struct {
    float gs, ge;
    OpeningKind kind;
} GapSpec;

static void build_wall(BlockBatch *wallB, SeededRng *wrng, char axis, float fixed, const GapSpec *gaps, int gapCount) {
    float min = -WORLD_EDGE, max = WORLD_EDGE;
    Opening *mine[4];
    int mineCount = 0;

    float cursor = min;
    float segs[8][2];
    int segCount = 0;

    for (int i = 0; i < gapCount; i++) {
        if (gaps[i].gs > cursor) {
            segs[segCount][0] = cursor;
            segs[segCount][1] = gaps[i].gs;
            segCount++;
        }
        if (gaps[i].ge > cursor) cursor = gaps[i].ge;
        mine[mineCount++] = make_opening(axis, fixed, gaps[i].gs, gaps[i].ge, gaps[i].kind, WALL_T);
    }
    if (cursor < max) {
        segs[segCount][0] = cursor;
        segs[segCount][1] = max;
        segCount++;
    }

    float layer0 = (axis == 'x') ? 0.24f : 0.44f;
    for (int i = 0; i < segCount; i++) {
        float a = segs[i][0], b = segs[i][1];
        float hw = (axis == 'x' ? b - a : WALL_T) * 0.5f;
        float hd = (axis == 'x' ? WALL_T : b - a) * 0.5f;
        float cx = (axis == 'x') ? (a + b) * 0.5f : fixed;
        float cz = (axis == 'x') ? fixed : (a + b) * 0.5f;
        if (gWorld.wallBoxCount < MAX_WALL_BOXES) {
            gWorld.wallBoxes[gWorld.wallBoxCount++] = (Box2D){ cx - hw, cx + hw, cz - hd, cz + hd };
        }
        log_run(wallB, wrng, axis, fixed, a, b, layer0, keep_always, NULL, a > min + 0.01f, b < max - 0.01f, true);
    }

    for (int i = 0; i < mineCount; i++) {
        Opening *o = mine[i];
        log_run(wallB, wrng, axis, fixed, o->u0, o->u1, layer0,
                o->kind == OPENING_DOOR ? keep_door : keep_window, o,
                true, true, o->kind == OPENING_WINDOW);
        fit_opening(o, wallB, wrng, gScene.litShader);
    }
}

static bool in_clearing(float x, float z, float pad) {
    if (fabsf(x) < WORLD_EDGE + 0.6f + pad && fabsf(z) < WORLD_EDGE + 0.6f + pad) return true;
    for (int i = 0; i < gWorld.clearingCount; i++) {
        Clearing c = gWorld.clearings[i];
        float dx = x - c.x, dz = z - c.z;
        if (dx * dx + dz * dz < (c.r + pad) * (c.r + pad)) return true;
    }
    return false;
}

static void add_clearing(float x, float z, float r) {
    if (gWorld.clearingCount < MAX_CLEARINGS) {
        gWorld.clearings[gWorld.clearingCount++] = (Clearing){ x, z, r };
    }
}

static int build_pine_blocks(BlockSpec *out, int tiers, float w0, float tierH, float trunkH) {
    int n = 0;
    const uint32_t TRUNK = 0x2b2018, NEEDLES = 0x1a2b1a, NEEDLE_TIPS = 0x2a4229;
    out[n++] = BLKB(0.32f, trunkH, 0.32f, 0, trunkH * 0.5f, 0, TRUNK, 0, 0, 0, 0.06f);
    float y = trunkH * 0.6f;
    for (int i = 0; i < tiers; i++) {
        float w = w0 * (1.0f - ((float)i / (float)tiers) * 0.75f);
        float ry = (i % 2) * PI * 0.25f;
        out[n++] = BLKB(w, tierH * 0.62f, w, 0, y, 0, NEEDLES, 0, ry, 0, 0.14f);
        out[n++] = BLKB(w * 0.6f, tierH * 0.42f, w * 0.6f, 0, y + tierH * 0.36f, 0, NEEDLE_TIPS, 0, ry, 0, 0.09f);
        y += tierH * 0.78f;
    }
    out[n++] = BLKB(0.3f, tierH * 0.7f, 0.3f, 0, y, 0, NEEDLE_TIPS, 0, 0, 0, 0.08f);
    return n;
}

static int build_oak_blocks(BlockSpec *out, SeededRng *rng) {
    int n = 0;
    out[n++] = BLKB(0.46f, 2.4f, 0.46f, 0, 1.2f, 0, 0x2b2018, 0, 0, 0, 0.08f);
    for (int i = 0; i < 6; i++) {
        float s = 1.3f + rng_next(rng) * 0.9f;
        float a = (float)i * 1.05f;
        float px = cosf(a) * 0.75f * (i ? 1.0f : 0.0f);
        float pz = sinf(a) * 0.75f * (i ? 1.0f : 0.0f);
        float py = 2.6f + rng_next(rng) * 1.1f + (i ? 0.0f : 0.8f);
        out[n++] = BLKB(s, s * 0.85f, s, px, py, pz, (i % 2) ? 0x24311c : 0x2e3d22, 0, rng_next(rng), 0, 0.2f);
    }
    return n;
}

static int build_dead_blocks(BlockSpec *out, SeededRng *rng) {
    int n = 0;
    out[n++] = BLKB(0.34f, 4.4f, 0.34f, 0, 2.2f, 0, 0x3d362e, 0, 0, 0, 0.06f);
    for (int i = 0; i < 6; i++) {
        float len = 0.9f + rng_next(rng) * 0.8f;
        float y0 = 1.8f + (float)i * 0.42f;
        float phi = 0.7f + rng_next(rng) * 0.4f;
        float side = (i % 2) ? 1.0f : -1.0f;
        if (i % 3 == 2) {
            out[n++] = BLKB(0.11f, len, 0.11f, 0, y0 + cosf(phi) * len * 0.5f, side * sinf(phi) * len * 0.5f,
                            0x3d362e, side * phi, 0, 0, 0.03f);
        } else {
            out[n++] = BLKB(0.11f, len, 0.11f, -side * sinf(phi) * len * 0.5f, y0 + cosf(phi) * len * 0.5f, 0,
                            0x3d362e, 0, 0, side * phi, 0.03f);
        }
    }
    return n;
}

void world_init(void) {
    gWorld = (GameWorld){0};
    solids_clear();
    gWorld.lanternPos = (Vector3){ 2.26f, 1.7f, 11.3f };
    gWorld.lanternIntensity = 7.0f;

    // Ground plane
    gWorld.groundTex = make_ground_texture();
    Mesh groundMesh = GenMeshPlane(200.0f, 200.0f, 1, 1);
    for (int i = 0; i < groundMesh.vertexCount; i++) {
        groundMesh.texcoords[i * 2 + 0] *= 25.0f;
        groundMesh.texcoords[i * 2 + 1] *= 25.0f;
    }
    UpdateMeshBuffer(groundMesh, 1, groundMesh.texcoords, groundMesh.vertexCount * 2 * (int)sizeof(float), 0);
    gWorld.groundModel = LoadModelFromMesh(groundMesh);
    gWorld.groundModel.materials[0].shader = gScene.litShader;
    gWorld.groundModel.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = gWorld.groundTex;

    // Cabin subfloor base
    BlockSpec baseBlk = BLKB(WORLD_HALF * 2.0f + WALL_T, 0.4f, WORLD_HALF * 2.0f + WALL_T, 0, -0.28f, 0, 0x150f0a, 0, 0, 0, 0.0f);
    SeededRng brng0 = rng_seed(1);
    gWorld.floorBaseModel = model_from_blocks(&baseBlk, 1, &brng0, true, gScene.litShader);

    // Cabin plank floor + rug + debris
    BlockBatch floorB;
    batch_init(&floorB);
    SeededRng frng = rng_seed(11);
    const float BOARD = 0.42f;
    const uint32_t boardTones[4] = { 0x4d3825, 0x46321f, 0x533d28, 0x41301e };
    for (float z = -WORLD_EDGE + BOARD * 0.5f; z < WORLD_EDGE; z += BOARD) {
        for (float x = -WORLD_EDGE - rng_next(&frng) * 2.0f; x < WORLD_EDGE;) {
            float len = 1.6f + rng_next(&frng) * 2.6f;
            float a = fmaxf(x, -WORLD_EDGE), b = fminf(x + len, WORLD_EDGE);
            float r = rng_next(&frng);
            uint32_t tone = r < 0.05f ? 0x2e2115 : (r < 0.1f ? 0x5e4730 : boardTones[(int)(rng_next(&frng) * 4.0f) % 4]);
            BlockSpec plank = BLKB(b - a - 0.015f, 0.08f, BOARD - 0.015f, (a + b) * 0.5f, -0.04f, z, tone, 0, 0, 0, 0.018f);
            batch_add(&floorB, &plank, 1, &frng, NULL, false);
            x += len;
        }
    }
    const float rugSpecs[4][5] = {
        {3.8f, 2.2f, 3.2f, 2.7f, (float)0x55201c},
        {3.2f, 1.6f, 2.6f, 2.1f, (float)0x6a5432},
        {2.6f, 1.0f, 2.0f, 1.5f, (float)0x55201c},
        {1.7f, 0.45f, 1.3f, 0.75f, (float)0x3e2618}
    };
    const uint32_t rugCols[4] = { 0x55201c, 0x6a5432, 0x55201c, 0x3e2618 };
    Matrix rugM = make_transform_yxz(3.0f, 0.0f, -2.6f, 0.0f, 0.08f, 0.0f);
    for (int i = 0; i < 4; i++) {
        BlockSpec rb[2] = {
            BLKB(rugSpecs[i][0], 0.012f, rugSpecs[i][1], 0, 0.006f + i * 0.008f, 0, rugCols[i], 0, 0, 0, 0.004f),
            BLKB(rugSpecs[i][2], 0.012f, rugSpecs[i][3], 0, 0.009f + i * 0.008f, 0, rugCols[i], 0, 0, 0, 0.004f)
        };
        batch_add(&floorB, rb, 2, &frng, &rugM, false);
    }
    const float pages[5][3] = { {-6.5f, -7.8f, 0.4f}, {-6.1f, -7.4f, 1.3f}, {7.2f, 6.4f, 2.2f}, {-8.2f, 3.2f, 0.9f}, {1.2f, 7.9f, 2.8f} };
    for (int i = 0; i < 5; i++) {
        Matrix pm = make_transform_yxz(pages[i][0], 0.0f, pages[i][1], 0.0f, pages[i][2], 0.0f);
        BlockSpec pg = BLKB(0.22f, 0.006f, 0.3f, 0, 0.004f, 0, 0xb5ab90, 0, 0, 0, 0.002f);
        batch_add(&floorB, &pg, 1, &frng, &pm, false);
    }
    Matrix canM = make_transform_yxz(-3.5f, 0.0f, 6.2f, 0.0f, 0.7f, 0.0f);
    BlockSpec canB[2] = {
        BLKB(0.13f, 0.13f, 0.17f, 0, 0.065f, 0, 0x8a8f96, 0, 0, 0, 0.02f),
        BLKB(0.135f, 0.07f, 0.1f, 0, 0.065f, 0.02f, 0x9a3a2a, 0, 0, 0, 0.01f)
    };
    batch_add(&floorB, canB, 2, &frng, &canM, false);
    const float stains[3][4] = { {0.4f, 8.9f, 0.9f, 0.2f}, {-0.5f, 8.1f, 0.6f, -0.4f}, {0.9f, 7.3f, 0.5f, 0.6f} };
    for (int i = 0; i < 3; i++) {
        Matrix sm = make_transform_yxz(stains[i][0], 0.0f, stains[i][1], 0.0f, stains[i][3], 0.0f);
        BlockSpec sb = BLKB(stains[i][2], 0.006f, stains[i][2] * 0.45f, 0, 0.004f, 0, 0x2a0b09, 0, 0, 0, 0.002f);
        batch_add(&floorB, &sb, 1, &frng, &sm, false);
    }
    gWorld.floorModel = LoadModelFromMesh(batch_build_mesh(&floorB));
    gWorld.floorModel.materials[0].shader = gScene.litShader;
    batch_free(&floorB);

    // Cabin walls + openings + interior props
    BlockBatch wallB;
    batch_init(&wallB);
    SeededRng wrng = rng_seed(23);
    GapSpec southGaps[1] = { {-0.8f, 0.8f, OPENING_DOOR} };
    GapSpec northGaps[2] = { {-5.8f, -4.2f, OPENING_DOOR}, {4.0f, 6.0f, OPENING_WINDOW} };
    GapSpec westGaps[1]  = { {-1.5f, 0.5f, OPENING_WINDOW} };
    GapSpec eastGaps[1]  = { {1.5f, 3.5f, OPENING_WINDOW} };
    build_wall(&wallB, &wrng, 'x',  WORLD_HALF, southGaps, 1);
    build_wall(&wallB, &wrng, 'x', -WORLD_HALF, northGaps, 2);
    build_wall(&wallB, &wrng, 'z', -WORLD_HALF, westGaps, 1);
    build_wall(&wallB, &wrng, 'z',  WORLD_HALF, eastGaps, 1);

    float IN_N = -WORLD_HALF + LOG_D * 0.5f;
    float IN_W = -WORLD_HALF + LOG_D * 0.5f;
    float IN_E =  WORLD_HALF - LOG_D * 0.5f;
    BlockSpec interior[22] = {
        BLKB(1.9f, 0.06f, 0.3f, -1.7f, 1.6f, IN_N + 0.15f, 0x5e4730, 0, 0, 0, 0.015f),
        BLK(0.05f, 0.22f, 0.26f, -2.5f, 1.48f, IN_N + 0.13f, POST),
        BLK(0.05f, 0.22f, 0.26f, -0.9f, 1.48f, IN_N + 0.13f, POST),
        BLKB(0.14f, 0.2f, 0.14f, -2.45f, 1.73f, IN_N + 0.15f, 0x6e7f4a, 0, 0, 0, 0.03f),
        BLKB(0.14f, 0.22f, 0.14f, -2.22f, 1.74f, IN_N + 0.15f, 0xa05a2a, 0, 0, 0, 0.03f),
        BLKB(0.12f, 0.16f, 0.12f, -1.95f, 1.71f, IN_N + 0.15f, 0x8a8f96, 0, 0, 0, 0.02f),
        BLKB(0.12f, 0.16f, 0.12f, -1.78f, 1.71f, IN_N + 0.13f, 0x8a8f96, 0, 0, 0, 0.02f),
        BLKB(0.32f, 0.08f, 0.22f, -1.25f, 1.67f, IN_N + 0.14f, 0x5a2a20, 0, 0.1f, 0, 0.015f),
        BLKB(0.3f, 0.07f, 0.2f, -1.24f, 1.745f, IN_N + 0.14f, 0x2a3a4a, 0, -0.15f, 0, 0.015f),
        BLKB(0.36f, 0.44f, 0.05f, 1.4f, 1.9f, IN_N + 0.025f, POST, 0, 0, 0, 0.02f),
        BLKB(0.17f, 0.2f, 0.16f, 1.4f, 1.9f, IN_N + 0.1f, 0xcfc4a2, 0, 0, 0, 0.03f),
        BLKB(0.04f, 0.42f, 0.04f, 1.27f, 2.13f, IN_N + 0.1f, 0xb8ab88, 0, 0, 0.55f, 0.01f),
        BLKB(0.04f, 0.42f, 0.04f, 1.53f, 2.13f, IN_N + 0.1f, 0xb8ab88, 0, 0, -0.55f, 0.01f),
        BLKB(0.03f, 0.18f, 0.03f, 1.15f, 2.25f, IN_N + 0.1f, 0xb8ab88, 0, 0, -0.3f, 0.008f),
        BLKB(0.03f, 0.18f, 0.03f, 1.65f, 2.25f, IN_N + 0.1f, 0xb8ab88, 0, 0, 0.3f, 0.008f),
        BLKB(0.48f, 0.62f, 0.02f, 2.75f, 1.4f, IN_N + 0.012f, 0xb8a888, 0, 0, 0.04f, 0.005f),
        BLKB(0.4f, 0.2f, 0.025f, 2.75f, 1.58f, IN_N + 0.015f, 0x7a2a22, 0, 0, 0.04f, 0.005f),
        BLKB(0.06f, 0.06f, 1.1f, IN_W + 0.04f, 1.75f, 4.6f, POST, 0, 0, 0, 0.015f),
        BLKB(0.16f, 0.85f, 0.5f, IN_W + 0.1f, 1.3f, 4.3f, 0x3e4a33, 0, 0, 0.04f, 0.04f),
        BLKB(0.24f, 0.12f, 0.24f, IN_W + 0.14f, 1.76f, 4.95f, 0x6b5232, 0, 0, 0, 0.04f),
        BLKB(0.03f, 0.5f, 0.42f, IN_E - 0.015f, 1.6f, -5.0f, POST, 0, 0, 0, 0.01f),
        BLKB(0.035f, 0.42f, 0.34f, IN_E - 0.02f, 1.6f, -5.0f, 0x55606a, 0, 0, 0, 0.005f),
    };
    batch_add(&wallB, interior, 22, &wrng, NULL, false);
    gWorld.wallModel = LoadModelFromMesh(batch_build_mesh(&wallB));
    gWorld.wallModel.materials[0].shader = gScene.litShader;
    batch_free(&wallB);

    // Yard props
    BlockBatch yard;
    batch_init(&yard);
    SeededRng yrng = rng_seed(41);
    for (int k = 0; k < 4; k++) {
        for (int i = 0; i < 10; i++) {
            float x = 3.75f + (float)i * 0.37f + (k % 2) * 0.18f;
            float y = 0.17f + (float)k * 0.3f;
            if (x > 7.5f) continue;
            BlockSpec fw[2] = {
                BLKB(0.33f, 0.3f, 0.62f, x, y, 10.9f, LOG_TONES[(int)(rng_next(&yrng) * 4.0f) % 4], 0, 0, (rng_next(&yrng) - 0.5f) * 0.2f, 0.1f),
                BLKB(0.27f, 0.24f, 0.02f, x, y, 11.22f, END_GRAIN, 0, 0, 0, 0.06f)
            };
            batch_add(&yard, fw, 2, &yrng, NULL, false);
        }
    }
    BlockSpec fwPosts[2] = { BLK(0.12f, 1.4f, 0.12f, 3.45f, 0.7f, 10.9f, POST), BLK(0.12f, 1.4f, 0.12f, 7.75f, 0.7f, 10.9f, POST) };
    batch_add(&yard, fwPosts, 2, &yrng, NULL, false);
    add_clearing(5.6f, 11.0f, 2.4f);

    Matrix chopM = make_transform_yxz(-4.3f, 0.0f, 12.5f, 0.0f, 0.6f, 0.0f);
    BlockSpec chopB[4] = {
        BLKB(0.62f, 0.5f, 0.62f, 0, 0.25f, 0, LOG_TONES[2], 0, 0, 0, 0.18f),
        BLKB(0.5f, 0.02f, 0.5f, 0, 0.505f, 0, END_GRAIN, 0, 0, 0, 0.12f),
        BLKB(0.06f, 0.06f, 0.78f, 0.06f, 0.82f, -0.2f, 0x6b4a2a, -0.75f, 0, 0, 0.02f),
        BLKB(0.06f, 0.2f, 0.28f, 0.06f, 0.56f, 0.06f, 0x5a5e66, -0.75f, 0, 0, 0.02f),
    };
    batch_add(&yard, chopB, 4, &yrng, &chopM, false);
    add_clearing(-4.3f, 12.5f, 1.5f);

    const uint32_t RUST = 0x46524c, RUST2 = 0x6a4a2c, CHROME = 0x7a7a72, GLASS = 0x182027;
    Matrix truckM = make_transform_yxz(-7.6f, 0.0f, 15.2f, 0.0f, 0.55f, 0.0f);
    BlockSpec truck[20] = {
        BLKB(1.92f, 0.62f, 4.6f, 0, 0.78f, 0, RUST, 0, 0, 0, 0.12f),
        BLKB(1.8f, 0.74f, 1.5f, 0, 1.45f, 0.45f, RUST, 0, 0, 0, 0.1f),
        BLKB(1.6f, 0.4f, 0.05f, 0, 1.52f, 1.21f, GLASS, -0.25f, 0, 0, 0.01f),
        BLKB(1.82f, 0.36f, 1.2f, 0, 1.55f, 0.4f, GLASS, 0, 0, 0, 0.02f),
        BLKB(1.86f, 0.2f, 1.4f, 0, 1.18f, 1.55f, RUST, 0, 0, 0, 0.08f),
        BLKB(0.1f, 0.42f, 2.0f, 0.92f, 1.28f, -1.2f, RUST, 0, 0, 0, 0.03f),
        BLKB(0.1f, 0.42f, 2.0f, -0.92f, 1.28f, -1.2f, RUST, 0, 0, 0, 0.03f),
        BLKB(1.94f, 0.42f, 0.1f, 0, 1.28f, -2.25f, RUST, 0, 0, 0.06f, 0.03f),
        BLKB(1.7f, 0.06f, 1.9f, 0, 1.1f, -1.2f, 0x2a2a28, 0, 0, 0, 0.01f),
        BLKB(2.02f, 0.2f, 0.16f, 0, 0.6f, 2.33f, CHROME, 0, 0, 0, 0.05f),
        BLKB(2.02f, 0.2f, 0.16f, 0, 0.6f, -2.33f, CHROME, 0, 0, 0, 0.05f),
        BLKB(0.32f, 0.2f, 0.05f, 0.62f, 0.95f, 2.31f, 0xe8e2b0, 0, 0, 0, 0.02f),
        BLKB(0.32f, 0.2f, 0.05f, -0.62f, 0.95f, 2.31f, 0x1a1a18, 0, 0, 0, 0.02f),
        BLKB(0.6f, 0.34f, 0.03f, 0.97f, 0.85f, 0.9f, RUST2, 0, 0, 0, 0.01f),
        BLKB(0.5f, 0.3f, 0.03f, -0.97f, 0.75f, -1.6f, RUST2, 0, 0, 0, 0.01f),
        BLKB(0.9f, 0.04f, 0.7f, 0.2f, 1.33f, -1.5f, 0x4a3a2a, 0, 0.3f, 0, 0.01f),
        BLKB(0.36f, 0.7f, 0.7f, 0.98f, 0.35f, 1.45f, 0x141414, 0, 0, 0, 0.2f),
        BLKB(0.36f, 0.7f, 0.7f, -0.98f, 0.35f, 1.45f, 0x141414, 0, 0, 0, 0.2f),
        BLKB(0.36f, 0.7f, 0.7f, 0.98f, 0.35f, -1.45f, 0x141414, 0, 0, 0, 0.2f),
        BLKB(0.36f, 0.56f, 0.78f, -0.98f, 0.26f, -1.45f, 0x141414, 0, 0, 0, 0.16f),
    };
    batch_add(&yard, truck, 20, &yrng, &truckM, false);
    add_clearing(-7.6f, 15.2f, 3.4f);
    add_solid(-7.6f, 15.2f, 0.55f, 1.05f, 2.45f);
    add_clearing(2.0f, 23.5f, 4.5f);
    add_clearing(1.0f, 17.5f, 3.5f);
    add_clearing(7.5f, 20.0f, 3.5f);

    BlockSpec lanternPost[3] = {
        BLK(0.16f, 2.3f, 0.16f, 2.7f, 1.15f, 11.3f, POST),
        BLK(0.56f, 0.08f, 0.08f, 2.48f, 2.22f, 11.3f, POST),
        BLK(0.24f, 0.06f, 0.24f, 2.26f, 2.0f, 11.3f, 0x2a2a2a),
    };
    batch_add(&yard, lanternPost, 3, &yrng, NULL, false);

    const float fences[3][5] = {
        {8.6f, 12.6f, 10.8f, 12.0f, 0},
        {10.8f, 12.0f, 12.6f, 10.6f, 1},
        {-12.5f, -9.5f, -12.2f, -12.0f, 0}
    };
    for (int i = 0; i < 3; i++) {
        float x0 = fences[i][0], z0 = fences[i][1], x1 = fences[i][2], z1 = fences[i][3];
        bool broken = fences[i][4] > 0.5f;
        float len = hypotf(x1 - x0, z1 - z0), ry = atan2f(x1 - x0, z1 - z0);
        Matrix m0 = make_transform_yxz(x0, 0.0f, z0, 0.0f, ry, 0.0f);
        Matrix m1 = make_transform_yxz(x1, 0.0f, z1, 0.0f, ry, 0.0f);
        BlockSpec p = BLK(0.14f, 1.2f, 0.14f, 0, 0.6f, 0, POST);
        batch_add(&yard, &p, 1, &yrng, &m0, false);
        batch_add(&yard, &p, 1, &yrng, &m1, false);
        if (broken) {
            BlockSpec r = BLKB(0.08f, 0.14f, len, 0, 0.5f, len * 0.5f, WEATHERED, 0.35f, 0, 0, 0.02f);
            batch_add(&yard, &r, 1, &yrng, &m0, false);
        } else {
            BlockSpec r[2] = {
                BLKB(0.08f, 0.14f, len, 0, 0.95f, len * 0.5f, WEATHERED, 0, 0, 0, 0.02f),
                BLKB(0.08f, 0.14f, len, 0, 0.5f, len * 0.5f, WEATHERED, 0, 0, 0, 0.02f)
            };
            batch_add(&yard, r, 2, &yrng, &m0, false);
        }
    }

    const float stumps[6][3] = {
        {11.6f, -4.0f, 1.0f}, {-11.4f, 7.5f, 0.8f}, {6.0f, -12.0f, 1.1f},
        {-12.6f, -2.5f, 0.9f}, {12.8f, 7.5f, 0.85f}, {-2.5f, -12.4f, 1.0f}
    };
    for (int i = 0; i < 6; i++) {
        float s = stumps[i][2];
        Matrix sm = make_transform_yxz(stumps[i][0], 0.0f, stumps[i][1], 0.0f, 0.0f, 0.0f);
        BlockSpec st[2] = {
            BLKB(0.55f * s, 0.4f * s, 0.55f * s, 0, 0.2f * s, 0, 0x3a2b1d, 0, rng_next(&yrng), 0, 0.15f),
            BLKB(0.46f * s, 0.02f, 0.46f * s, 0, 0.4f * s + 0.01f, 0, END_GRAIN, 0, rng_next(&yrng), 0, 0.12f)
        };
        batch_add(&yard, st, 2, &yrng, &sm, false);
    }

    Matrix logM = make_transform_yxz(-13.5f, 0.0f, 4.0f, 0.0f, 0.3f, 0.0f);
    BlockSpec fallen[3] = {
        BLKB(0.6f, 0.55f, 5.5f, 0, 0.27f, 0, 0x3a2f24, 0, 0, 0, 0.18f),
        BLKB(0.5f, 0.45f, 0.02f, 0, 0.27f, 2.76f, END_GRAIN, 0, 0, 0, 0.12f),
        BLKB(0.14f, 0.9f, 0.14f, 0.3f, 0.6f, -1.2f, 0x3a2f24, 0, 0, -0.7f, 0.03f)
    };
    batch_add(&yard, fallen, 3, &yrng, &logM, false);
    add_clearing(-13.5f, 4.0f, 3.0f);

    gWorld.yardModel = LoadModelFromMesh(batch_build_mesh(&yard));
    gWorld.yardModel.materials[0].shader = gScene.litShader;
    batch_free(&yard);

    BlockSpec lgBlk = BLKB(0.2f, 0.26f, 0.2f, 0, 0, 0, 0x2a1a08, 0, 0, 0, 0.0f);
    SeededRng lrng = rng_seed(2);
    gWorld.lanternGlassModel = model_from_blocks(&lgBlk, 1, &lrng, true, gScene.litShader);

    // Trees
    SeededRng trng = rng_seed(77);
    BlockSpec tBlocks[16];
    int tc;
    SeededRng bakeRng;

    tc = build_pine_blocks(tBlocks, 4, 3.0f, 1.05f, 1.6f);
    bakeRng = rng_seed(1); gWorld.treeModelsNear[0] = model_from_blocks(tBlocks, tc, &bakeRng, false, gScene.litShader);
    bakeRng = rng_seed(1); gWorld.treeModelsFar[0]  = model_from_blocks(tBlocks, tc, &bakeRng, true, gScene.litShader);

    tc = build_pine_blocks(tBlocks, 3, 2.6f, 0.95f, 1.2f);
    bakeRng = rng_seed(1); gWorld.treeModelsNear[1] = model_from_blocks(tBlocks, tc, &bakeRng, false, gScene.litShader);
    bakeRng = rng_seed(1); gWorld.treeModelsFar[1]  = model_from_blocks(tBlocks, tc, &bakeRng, true, gScene.litShader);

    tc = build_pine_blocks(tBlocks, 6, 1.8f, 0.75f, 1.3f);
    bakeRng = rng_seed(1); gWorld.treeModelsNear[2] = model_from_blocks(tBlocks, tc, &bakeRng, false, gScene.litShader);
    bakeRng = rng_seed(1); gWorld.treeModelsFar[2]  = model_from_blocks(tBlocks, tc, &bakeRng, true, gScene.litShader);

    tc = build_oak_blocks(tBlocks, &trng);
    bakeRng = rng_seed(1); gWorld.treeModelsNear[3] = model_from_blocks(tBlocks, tc, &bakeRng, false, gScene.litShader);
    bakeRng = rng_seed(1); gWorld.treeModelsFar[3]  = model_from_blocks(tBlocks, tc, &bakeRng, true, gScene.litShader);

    tc = build_dead_blocks(tBlocks, &trng);
    bakeRng = rng_seed(1); gWorld.treeModelsNear[4] = model_from_blocks(tBlocks, tc, &bakeRng, false, gScene.litShader);
    bakeRng = rng_seed(1); gWorld.treeModelsFar[4]  = model_from_blocks(tBlocks, tc, &bakeRng, true, gScene.litShader);

    const float treeWeights[5] = { 38, 18, 22, 13, 9 };
    const float treeTotal = 100.0f;
    for (int i = 0; i < FOREST_COUNT; i++) {
        float rad = sqrtf(13.0f * 13.0f + rnd01() * (70.0f * 70.0f - 13.0f * 13.0f));
        float ang = rnd01() * 2.0f * PI;
        float x = cosf(ang) * rad, z = sinf(ang) * rad;
        if (in_clearing(x, z, 0.8f)) continue;
        float r = rnd01() * treeTotal;
        int k = 0;
        while (k < 4 && (r -= treeWeights[k]) > 0.0f) k++;
        float s = 0.85f + rnd01() * 0.45f;
        float sy = s * (0.9f + rnd01() * 0.3f);
        Matrix mS = MatrixScale(s, sy, s);
        Matrix mR = MatrixRotateY(rnd01() * 2.0f * PI);
        Matrix mT = MatrixTranslate(x, GROUND_Y, z);
        uint8_t tv = (uint8_t)((0.8f + rnd01() * 0.3f) * 255.0f);
        gWorld.trees[gWorld.treeCount++] = (PropInstance){
            .transform = MatrixMultiply(MatrixMultiply(mS, mR), mT),
            .tint = (Color){ tv, tv, tv, 255 },
            .kind = k, .x = x, .z = z, .far = rad >= 24.0f
        };
    }

    // Brush
    SeededRng brushRng = rng_seed(91);
    BlockSpec tuft[6];
    for (int i = 0; i < 6; i++) {
        float a = (float)i * 1.05f + rng_next(&brushRng);
        float h = 0.25f + rng_next(&brushRng) * 0.25f;
        tuft[i] = BLKB(0.05f, h, 0.05f, cosf(a) * 0.07f, h * 0.5f, sinf(a) * 0.07f,
                       rng_next(&brushRng) < 0.5f ? 0x2d3b1e : 0x3a4826, sinf(a) * 0.35f, 0, -cosf(a) * 0.35f, 0.0f);
    }
    gWorld.brushModels[0] = model_from_blocks(tuft, 6, &brushRng, true, gScene.litShader);

    BlockSpec fern[7];
    for (int i = 0; i < 7; i++) {
        float a = (float)i * 0.9f;
        fern[i] = BLKB(0.11f, 0.03f, 0.6f, sinf(a) * 0.28f, 0.12f, cosf(a) * 0.28f, 0x2c4a22, -0.35f, a, 0, 0.0f);
    }
    gWorld.brushModels[1] = model_from_blocks(fern, 7, &brushRng, true, gScene.litShader);

    BlockSpec rock[2] = {
        BLKB(0.5f, 0.32f, 0.42f, 0, 0.12f, 0, 0x3a3834, 0, 0.3f, 0.1f, 0.1f),
        BLKB(0.3f, 0.22f, 0.3f, 0.24f, 0.08f, 0.14f, 0x33312d, 0, 0.9f, 0, 0.08f)
    };
    gWorld.brushModels[2] = model_from_blocks(rock, 2, &brushRng, false, gScene.litShader);

    const float brushWeights[3] = { 60, 25, 15 };
    for (int i = 0; i < BRUSH_COUNT; i++) {
        float rad = sqrtf(10.6f * 10.6f + rnd01() * (28.0f * 28.0f - 10.6f * 10.6f));
        float ang = rnd01() * 2.0f * PI;
        float x = cosf(ang) * rad, z = sinf(ang) * rad;
        if (in_clearing(x, z, 0.0f)) continue;
        float r = rnd01() * 100.0f;
        int k = 0;
        while (k < 2 && (r -= brushWeights[k]) > 0.0f) k++;
        float s = 0.7f + rnd01() * 0.7f;
        float sy = s * (0.9f + rnd01() * 0.3f);
        Matrix mS = MatrixScale(s, sy, s);
        Matrix mR = MatrixRotateY(rnd01() * 2.0f * PI);
        Matrix mT = MatrixTranslate(x, GROUND_Y, z);
        uint8_t tv = (uint8_t)((0.75f + rnd01() * 0.4f) * 255.0f);
        gWorld.brush[gWorld.brushCount++] = (PropInstance){
            .transform = MatrixMultiply(MatrixMultiply(mS, mR), mT),
            .tint = (Color){ tv, tv, tv, 255 },
            .kind = k, .x = x, .z = z, .far = false
        };
    }

    gWorld.moonTex = make_moon_texture();
    init_solids_nav();
}

void world_update(float dt, float weatherLevel) {
    (void)dt;
    float t = (float)GetTime();
    float f = 0.85f + 0.08f * sinf(t * 7.3f) + 0.06f * sinf(t * 13.1f + 1.0f)
              - (sinf(t * 0.9f) > 0.97f ? 0.35f : 0.0f)
              - weatherLevel * 0.2f * (0.5f + 0.5f * sinf(t * 5.3f) * sinf(t * 2.1f));
    gWorld.lanternIntensity = 1.6f * f;
}

void world_draw(Vector3 camTarget) {
    // Ground with texture
    int useTex = 1;
    SetShaderValue(gScene.litShader, gScene.locUseTex, &useTex, SHADER_UNIFORM_INT);
    DrawModel(gWorld.groundModel, (Vector3){ 0.0f, GROUND_Y, 0.0f }, 1.0f, WHITE);
    useTex = 0;
    SetShaderValue(gScene.litShader, gScene.locUseTex, &useTex, SHADER_UNIFORM_INT);

    DrawModel(gWorld.floorBaseModel, (Vector3){0}, 1.0f, WHITE);
    DrawModel(gWorld.floorModel, (Vector3){0}, 1.0f, WHITE);
    DrawModel(gWorld.wallModel, (Vector3){0}, 1.0f, WHITE);
    DrawModel(gWorld.yardModel, (Vector3){0}, 1.0f, WHITE);

    // Lantern glass with emissive glow
    float em = 0.55f * (gWorld.lanternIntensity / 1.6f);
    float emVec[3] = { 1.0f * em, 0.69f * em, 0.31f * em };
    SetShaderValue(gScene.litShader, gScene.locEmissive, emVec, SHADER_UNIFORM_VEC3);
    DrawModel(gWorld.lanternGlassModel, (Vector3){ 2.26f, 1.84f, 11.3f }, 1.0f, WHITE);
    float zero[3] = { 0, 0, 0 };
    SetShaderValue(gScene.litShader, gScene.locEmissive, zero, SHADER_UNIFORM_VEC3);

    // Trees & brush within fog range
    float maxD2 = (gScene.fogFar + 8.0f) * (gScene.fogFar + 8.0f);
    for (int i = 0; i < gWorld.treeCount; i++) {
        PropInstance *p = &gWorld.trees[i];
        float dx = p->x - camTarget.x, dz = p->z - camTarget.z;
        if (dx * dx + dz * dz > maxD2) continue;
        Model *m = p->far ? &gWorld.treeModelsFar[p->kind] : &gWorld.treeModelsNear[p->kind];
        DrawMesh(m->meshes[0], m->materials[0], p->transform);
    }
    for (int i = 0; i < gWorld.brushCount; i++) {
        PropInstance *p = &gWorld.brush[i];
        float dx = p->x - camTarget.x, dz = p->z - camTarget.z;
        if (dx * dx + dz * dz > maxD2) continue;
        Model *m = &gWorld.brushModels[p->kind];
        DrawMesh(m->meshes[0], m->materials[0], p->transform);
    }

    draw_openings(gScene.litShader);
}

void world_cleanup(void) {
    UnloadModel(gWorld.groundModel);
    UnloadTexture(gWorld.groundTex);
    UnloadModel(gWorld.floorBaseModel);
    UnloadModel(gWorld.floorModel);
    UnloadModel(gWorld.wallModel);
    UnloadModel(gWorld.yardModel);
    UnloadModel(gWorld.lanternGlassModel);
    for (int i = 0; i < 5; i++) {
        UnloadModel(gWorld.treeModelsNear[i]);
        UnloadModel(gWorld.treeModelsFar[i]);
    }
    for (int i = 0; i < 3; i++) {
        UnloadModel(gWorld.brushModels[i]);
    }
    UnloadTexture(gWorld.moonTex);
    unload_openings();
}
