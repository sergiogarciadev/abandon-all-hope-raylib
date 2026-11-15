#include "player.h"
#include "animation.h"
#include "core/blocks.h"
#include "core/math_util.h"
#include "core/scene.h"
#include "core/state.h"
#include "effects/particles.h"
#include "rlgl.h"
#include <math.h>

PlayerRig gPlayer = {0};

#define P_THIGH 0.44f
#define P_SHIN 0.42f
#define P_ANKLE_H 0.15f
#define P_HIP_H 0.97f
#define P_UPPER_ARM 0.42f
#define P_FOREARM 0.42f

#define BETA 0.38f
#define HIP_MAX 1.0f
#define IDLE_HIP -0.35f
#define BLADE -0.62f
#define PLAYER_BASE_SPEED 6.2f

static const Vector3 POCKET = { -0.13f, 0.6f, 0.22f };
static const Vector3 POUCH  = {  0.17f, 0.33f, 0.36f };
static const Vector3 GRIP_R = {  0.0f, -0.1f, 0.3f };
static const Vector3 GRIP_L = {  0.0f, -0.07f, 0.74f };
static const Vector3 MAG_BASE = { 0.0f, -0.29f, 0.53f };
static const Vector3 KNEE_POLE = { 0.0f, 0.0f, 1.0f };

static Texture2D make_plaid_texture(void) {
    Image img = GenImageColor(8, 8, hex_color(0xa8332b, 255));
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            bool warp = i < 4, weft = j < 4;
            uint32_t hex = (warp && weft) ? 0x191314 : ((warp || weft) ? 0x5c1b18 : 0xa8332b);
            ImageDrawPixel(&img, i, j, hex_color(hex, 255));
        }
    }
    Texture2D tex = LoadTextureFromImage(img);
    UnloadImage(img);
    SetTextureWrap(tex, TEXTURE_WRAP_REPEAT);
    SetTextureFilter(tex, TEXTURE_FILTER_POINT);
    return tex;
}

void player_init(void) {
    gPlayer = (PlayerRig){0};
    gPlayer.visible = true;
    gPlayer.stride = (Vector3){ 0, 0, 1 };
    gPlayer.flashColor = hex_color(0xffa64d, 255);
    gPlayer.plaidTex = make_plaid_texture();

    SeededRng rng = rng_seed(101);

    // Pelvis
    BlockSpec pelvisB[5] = {
        BLK(0.62f, 0.3f, 0.4f, 0, 0.02f, 0, 0x39404d),
        BLKB(0.66f, 0.08f, 0.43f, 0, 0.12f, 0, 0x5e3f27, 0, 0, 0, 0.02f),
        BLKB(0.1f, 0.07f, 0.03f, 0, 0.12f, 0.22f, 0xc9a14a, 0, 0, 0, 0.01f),
        BLK(0.12f, 0.17f, 0.17f, 0.36f, 0.02f, 0.04f, 0x3a3e27),
        BLK(0.07f, 0.28f, 0.1f, -0.35f, -0.04f, -0.03f, 0x5e3f27),
    };
    gPlayer.pelvisModel = model_from_blocks(pelvisB, 5, &rng, false, gScene.litShader);

    // Legs
    BlockSpec thighB = BLK(0.26f, P_THIGH + 0.08f, 0.29f, 0, 0, 0, 0x39404d);
    gPlayer.thighModel = model_from_blocks(&thighB, 1, &rng, false, gScene.litShader);

    BlockSpec shinB[2] = {
        BLK(0.24f, P_SHIN + 0.06f, 0.27f, 0, 0, 0, 0x39404d),
        BLK(0.2f, 0.17f, 0.07f, 0, P_SHIN * 0.5f - 0.03f, 0.15f, 0x3a3e27),
    };
    gPlayer.shinModel = model_from_blocks(shinB, 2, &rng, false, gScene.litShader);

    BlockSpec bootB[3] = {
        BLK(0.27f, 0.17f, 0.29f, 0, 0, -0.01f, 0x5e3f27),
        BLK(0.27f, 0.1f, 0.42f, 0, -0.08f, 0.06f, 0x5e3f27),
        BLKB(0.29f, 0.05f, 0.45f, 0, -0.13f, 0.06f, 0x17130f, 0, 0, 0, 0.015f),
    };
    gPlayer.bootModel = model_from_blocks(bootB, 3, &rng, false, gScene.litShader);

    // Upper body: flannel (textured) + gear (vertex colored)
    BlockSpec flannelB[2] = {
        BLK(0.74f, 0.6f, 0.42f, 0, 0.36f, 0, 0xffffff),
        BLK(0.84f, 0.26f, 0.46f, 0, 0.62f, 0, 0xffffff),
    };
    gPlayer.upperFlannelModel = model_from_blocks(flannelB, 2, &rng, false, gScene.litShader);
    gPlayer.upperFlannelModel.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = gPlayer.plaidTex;

    BlockSpec gearB[13] = {
        BLK(0.5f, 0.1f, 0.4f, 0, 0.78f, -0.02f, 0xa88d68),
        BLK(0.6f, 0.4f, 0.07f, 0, 0.38f, 0.22f, 0x515737),
        BLK(0.15f, 0.17f, 0.08f, -0.17f, 0.33f, 0.28f, 0x3a3e27),
        BLK(0.15f, 0.17f, 0.08f,  0.0f,  0.33f, 0.28f, 0x3a3e27),
        BLK(0.15f, 0.17f, 0.08f,  0.17f, 0.33f, 0.28f, 0x3a3e27),
        BLK(0.6f, 0.48f, 0.06f, 0, 0.42f, -0.23f, 0x515737),
        BLK(0.12f, 0.05f, 0.5f, -0.21f, 0.77f, 0, 0x515737),
        BLK(0.12f, 0.05f, 0.5f,  0.21f, 0.77f, 0, 0x515737),
        BLK(0.48f, 0.42f, 0.18f, 0, 0.36f, -0.33f, 0x6f5d3f),
        BLK(0.3f, 0.14f, 0.06f, 0, 0.27f, -0.44f, 0x3a3e27),
        BLK(0.68f, 0.17f, 0.17f, 0, 0.66f, -0.36f, 0x3e4a36),
        BLKB(0.05f, 0.19f, 0.19f, -0.2f, 0.66f, -0.36f, 0x5e3f27, 0, 0, 0, 0.01f),
        BLKB(0.05f, 0.19f, 0.19f,  0.2f, 0.66f, -0.36f, 0x5e3f27, 0, 0, 0, 0.01f),
    };
    gPlayer.upperGearModel = model_from_blocks(gearB, 13, &rng, false, gScene.litShader);

    // Head
    BlockSpec headB[16] = {
        BLK(0.2f, 0.14f, 0.2f, 0, 0.04f, 0, 0xe0915c),
        BLK(0.52f, 0.5f, 0.5f, 0, 0.32f, 0, 0xe0915c),
        BLK(0.54f, 0.2f, 0.2f, 0, 0.36f, -0.17f, 0x3a2517),
        BLKB(0.05f, 0.2f, 0.3f, -0.25f, 0.19f, 0.08f, 0x5b3920, 0, 0, 0, 0.015f),
        BLKB(0.05f, 0.2f, 0.3f,  0.25f, 0.19f, 0.08f, 0x5b3920, 0, 0, 0, 0.015f),
        BLKB(0.46f, 0.12f, 0.06f, 0, 0.12f, 0.24f, 0x5b3920, 0, 0, 0, 0.02f),
        BLKB(0.2f, 0.04f, 0.03f, 0, 0.21f, 0.26f, 0x5b3920, 0, 0, 0, 0.01f),
        BLKB(0.08f, 0.1f, 0.07f, 0, 0.27f, 0.27f, 0xe0915c, 0, 0, 0, 0.015f),
        BLKB(0.07f, 0.07f, 0.02f, -0.12f, 0.34f, 0.255f, 0x16100d, 0, 0, 0, 0.005f),
        BLKB(0.07f, 0.07f, 0.02f,  0.12f, 0.34f, 0.255f, 0x16100d, 0, 0, 0, 0.005f),
        BLKB(0.13f, 0.04f, 0.03f, -0.12f, 0.41f, 0.255f, 0x3a2517, 0, 0, 0, 0.008f),
        BLKB(0.13f, 0.04f, 0.03f,  0.12f, 0.41f, 0.255f, 0x3a2517, 0, 0, 0, 0.008f),
        BLKB(0.05f, 0.11f, 0.1f, -0.27f, 0.32f, 0, 0xe0915c, 0, 0, 0, 0.012f),
        BLKB(0.05f, 0.11f, 0.1f,  0.27f, 0.32f, 0, 0xe0915c, 0, 0, 0, 0.012f),
        BLK(0.58f, 0.1f, 0.56f, 0, 0.49f, -0.01f, 0xb9501f),
        BLK(0.55f, 0.2f, 0.53f, 0, 0.6f, -0.01f, 0xe0662a),
    };
    gPlayer.headModel = model_from_blocks(headB, 16, &rng, false, gScene.litShader);

    // Rifle
    BlockSpec gunB[16] = {
        BLKB(0.13f, 0.19f, 0.04f, 0, -0.02f, 0.02f, 0x151518, 0, 0, 0, 0.01f),
        BLK(0.12f, 0.17f, 0.24f, 0, -0.02f, 0.15f, 0x7b4a29),
        BLKR(0.1f, 0.2f, 0.1f, 0, -0.11f, 0.29f, 0x7b4a29, 0.35f, 0, 0),
        BLK(0.14f, 0.15f, 0.4f, 0, 0.03f, 0.44f, 0x2b2e34),
        BLKB(0.04f, 0.03f, 0.12f, 0, -0.07f, 0.37f, 0x2b2e34, 0, 0, 0, 0.01f),
        BLK(0.13f, 0.13f, 0.36f, 0, 0.01f, 0.8f, 0x7b4a29),
        BLKB(0.06f, 0.06f, 0.46f, 0, 0.03f, 1.18f, 0x2b2e34, 0, 0, 0, 0.015f),
        BLKB(0.085f, 0.085f, 0.08f, 0, 0.03f, 1.42f, 0x2b2e34, 0, 0, 0, 0.015f),
        BLKB(0.03f, 0.07f, 0.03f, 0, 0.09f, 1.34f, 0x2b2e34, 0, 0, 0, 0.008f),
        BLKB(0.075f, 0.075f, 0.4f, 0, 0.17f, 0.46f, 0x151518, 0, 0, 0, 0.02f),
        BLKB(0.1f, 0.1f, 0.08f, 0, 0.17f, 0.28f, 0x151518, 0, 0, 0, 0.02f),
        BLKB(0.11f, 0.11f, 0.09f, 0, 0.17f, 0.66f, 0x151518, 0, 0, 0, 0.02f),
        BLKB(0.08f, 0.08f, 0.01f, 0, 0.17f, 0.706f, 0x2a6fb8, 0, 0, 0, 0.004f),
        BLKB(0.04f, 0.07f, 0.04f, 0, 0.11f, 0.38f, 0x2b2e34, 0, 0, 0, 0.01f),
        BLKB(0.04f, 0.07f, 0.04f, 0, 0.11f, 0.56f, 0x2b2e34, 0, 0, 0, 0.01f),
        BLKB(0.18f, 0.06f, 0.18f, 0, 0.72f, -0.01f, 0xe0662a, 0, 0, 0, 0.02f), // knit top unused slot -> keep 15
    };
    gPlayer.gunModel = model_from_blocks(gunB, 15, &rng, false, gScene.litShader);

    BlockSpec magB = BLKR(0.09f, 0.24f, 0.12f, 0, 0, 0, 0x2b2e34, -0.12f, 0, 0);
    gPlayer.magModel = model_from_blocks(&magB, 1, &rng, false, gScene.litShader);

    // Arms
    BlockSpec upArmB[2] = {
        BLK(0.2f, P_UPPER_ARM + 0.1f, 0.21f, 0, 0, 0, 0xffffff),
        BLK(0.25f, 0.24f, 0.25f, 0, P_UPPER_ARM * 0.5f, 0, 0xffffff),
    };
    gPlayer.upperArmModel = model_from_blocks(upArmB, 2, &rng, false, gScene.litShader);
    gPlayer.upperArmModel.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = gPlayer.plaidTex;

    BlockSpec foreB[2] = {
        BLK(0.18f, P_FOREARM + 0.08f, 0.19f, 0, 0, 0, 0xffffff),
        BLK(0.21f, 0.08f, 0.22f, 0, -P_FOREARM * 0.5f + 0.05f, 0, 0xb99d74),
    };
    gPlayer.forearmModel = model_from_blocks(foreB, 2, &rng, false, gScene.litShader);
    gPlayer.forearmModel.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = gPlayer.plaidTex;

    BlockSpec handB = BLK(0.14f, 0.15f, 0.17f, 0, 0, 0, 0xe0915c);
    gPlayer.handModel = model_from_blocks(&handB, 1, &rng, false, gScene.litShader);
}

void player_muzzle_flash(bool hot) {
    gPlayer.kick = 1.0f;
    gPlayer.flash = 1.0f;
    gPlayer.flashPeak = 4.5f + rndf(0.0f, 1.5f);
    gPlayer.flashColor = hex_color(hot ? 0xff6a2a : 0xffa64d, 255);
    gPlayer.flashRotZ = rndf(0.0f, PI);
    gPlayer.flashScale = 0.85f + rndf(0.0f, 0.4f);
}

static Vector3 xform_pt(Vector3 p, Matrix m) {
    return (Vector3){
        m.m0 * p.x + m.m4 * p.y + m.m8  * p.z + m.m12,
        m.m1 * p.x + m.m5 * p.y + m.m9  * p.z + m.m13,
        m.m2 * p.x + m.m6 * p.y + m.m10 * p.z + m.m14
    };
}

static Vector3 reload_hand(float u, Matrix mGun, Matrix mUpper) {
    struct { float u; Vector3 p; bool isGun; } path[7] = {
        { 0.00f, GRIP_L, true },
        { 0.12f, MAG_BASE, true },
        { 0.22f, { MAG_BASE.x, MAG_BASE.y - 0.24f, MAG_BASE.z }, true },
        { 0.45f, POUCH, false },
        { 0.55f, POUCH, false },
        { 0.78f, { MAG_BASE.x, MAG_BASE.y - 0.20f, MAG_BASE.z }, true },
        { 0.95f, MAG_BASE, true },
    };
    int i = 6;
    while (i > 0 && u < path[i].u) i--;
    Vector3 a = xform_pt(path[i].p, path[i].isGun ? mGun : mUpper);
    if (i == 6) return a;
    Vector3 b = xform_pt(path[i + 1].p, path[i + 1].isGun ? mGun : mUpper);
    float t = (u - path[i].u) / (path[i + 1].u - path[i].u);
    return Vector3Lerp(a, b, ease_smooth(t));
}

void player_animate(float dt, float mx, float mz) {
    float spd = gState.speed > 0.1f ? gState.speed : PLAYER_BASE_SPEED;
    float pace = fminf(1.0f, hypotf(mx, mz) / spd);
    bool moving = pace > 0.05f;
    float aim = gPlayer.rotY;
    gPlayer.t += dt;
    gPlayer.gait = dampf(gPlayer.gait, pace, moving ? 9.0f : 7.0f, dt);

    float moveYaw = atan2f(mx, mz);
    float hipTarget = gPlayer.hipYaw;
    if (moving) {
        float rel = wrap_angle(moveYaw - aim);
        if (gPlayer.back ? fabsf(rel) < 1.25f : fabsf(rel) > 1.9f) gPlayer.back = !gPlayer.back;
        float clamped = clampf(gPlayer.back ? wrap_angle(rel - PI) : rel, -HIP_MAX, HIP_MAX);
        hipTarget = aim + clamped;
        gPlayer.turning = false;
    } else {
        float idle = aim + IDLE_HIP;
        if (fabsf(wrap_angle(idle - gPlayer.hipYaw)) > 0.8f) gPlayer.turning = true;
        if (gPlayer.turning) {
            hipTarget = idle;
            if (fabsf(wrap_angle(idle - gPlayer.hipYaw)) < 0.06f) gPlayer.turning = false;
        }
    }
    gPlayer.hipYaw = wrap_angle(gPlayer.hipYaw + wrap_angle(hipTarget - gPlayer.hipYaw) * (1.0f - expf(-10.0f * dt)));
    float hipRel = wrap_angle(gPlayer.hipYaw - aim);
    gPlayer.mRoot = make_transform_yxz(gPlayer.position.x, gPlayer.position.y, gPlayer.position.z, 0.0f, aim, 0.0f);
    gPlayer.mLower = make_transform_yxz(0.0f, 0.0f, 0.0f, 0.0f, hipRel, 0.0f);

    if (moving) {
        float s = moveYaw - gPlayer.hipYaw;
        gPlayer.stride = (Vector3){ sinf(s), 0.0f, cosf(s) };
    }

    gPlayer.turnW = dampf(gPlayer.turnW, gPlayer.turning ? 1.0f : 0.0f, 12.0f, dt);
    float drive = fmaxf(gPlayer.gait, gPlayer.turnW * 0.7f);
    float freq = 2.45f * sqrtf(spd / PLAYER_BASE_SPEED);
    if (drive > 0.02f) gPlayer.phase = fmodf(gPlayer.phase + freq * dt, 1.0f);
    float ph = gPlayer.phase * 2.0f * PI;
    float L = fminf(0.86f, spd * BETA / freq) * gPlayer.gait;
    float liftH = 0.2f * gPlayer.gait + 0.1f * gPlayer.turnW;

    float bob = -0.04f * gPlayer.gait * cosf(2.0f * (ph - PI * BETA));
    float pelvisY = P_HIP_H - 0.11f * gPlayer.gait + bob;
    Matrix mPelvisLocal = make_transform_yxz(0.0f, pelvisY, 0.0f,
                                             0.0f,
                                             0.14f * gPlayer.gait * sinf(ph) * gPlayer.stride.z,
                                             0.05f * gPlayer.gait * sinf(ph));
    Matrix mLowerWorld = MatrixMultiply(gPlayer.mLower, gPlayer.mRoot);
    gPlayer.mPelvis = MatrixMultiply(mPelvisLocal, mLowerWorld);

    for (int i = 0; i < 2; i++) {
        float side = (i == 0) ? 1.0f : -1.0f;
        float p = fmodf(gPlayer.phase + i * 0.5f, 1.0f);
        FootPose fp = foot_cycle(p, BETA);
        float restZ = side > 0.0f ? 0.12f : -0.1f;
        Vector3 foot = {
            side * (0.17f + 0.03f * (1.0f - gPlayer.gait)) + gPlayer.stride.x * fp.s * L,
            P_ANKLE_H + fp.lift * liftH,
            restZ * (1.0f - gPlayer.gait) + gPlayer.stride.z * fp.s * L
        };
        Vector3 hipP = xform_pt((Vector3){ side * 0.16f, 0.0f, 0.0f }, mPelvisLocal);
        Vector3 knee = ik2(hipP, foot, P_THIGH, P_SHIN, KNEE_POLE);
        Vector3 ankle = Vector3Add(knee, Vector3Scale(Vector3Normalize(Vector3Subtract(foot, knee)), P_SHIN));
        gPlayer.mThigh[i] = MatrixMultiply(bone_transform(hipP, knee, KNEE_POLE), mLowerWorld);
        gPlayer.mShin[i]  = MatrixMultiply(bone_transform(knee, ankle, KNEE_POLE), mLowerWorld);
        float sgnZ = gPlayer.stride.z >= 0.0f ? 1.0f : -1.0f;
        Matrix mBootLocal = make_transform_yxz(ankle.x, ankle.y, ankle.z,
                                               fp.pitch * gPlayer.gait * sgnZ, side * 0.08f, 0.0f);
        gPlayer.mBoot[i] = MatrixMultiply(mBootLocal, mLowerWorld);
    }

    float breathe = sinf(gPlayer.t * 2.1f) * (1.0f - gPlayer.gait);
    float tw = BLADE + 0.3f * (hipRel - BLADE) - 0.07f * gPlayer.gait * sinf(ph) * gPlayer.stride.z;
    float travel = moveYaw - (aim + tw);
    gPlayer.leanF = dampf(gPlayer.leanF, moving ? cosf(travel) : 0.0f, 8.0f, dt);
    gPlayer.leanS = dampf(gPlayer.leanS, moving ? sinf(travel) : 0.0f, 8.0f, dt);
    gPlayer.kick *= expf(-14.0f * dt);

    Matrix mUpperLocal = make_transform_yxz(0.0f, pelvisY + 0.1f + 0.006f * breathe, 0.0f,
                                            0.05f + 0.1f * gPlayer.leanF * gPlayer.gait - 0.08f * gPlayer.kick,
                                            tw,
                                            -0.07f * gPlayer.leanS * gPlayer.gait);
    gPlayer.mUpper = MatrixMultiply(mUpperLocal, gPlayer.mRoot);

    gPlayer.reload = dampf(gPlayer.reload, gState.reloading ? 1.0f : 0.0f, gState.reloading ? 14.0f : 9.0f, dt);
    float rl = ease_smooth(gPlayer.reload);
    float u = gState.reloading ? (gState.reloadT / gState.reloadTime) : 1.0f;

    Matrix mHeadLocal = make_transform_yxz(0.0f, 0.8f, 0.02f, 0.1f + 0.32f * rl, -tw * 0.85f + 0.25f * rl, 0.0f);
    gPlayer.mHead = MatrixMultiply(mHeadLocal, gPlayer.mUpper);

    Vector3 pocket = xform_pt(POCKET, mUpperLocal);
    Matrix mGunLocal = make_transform_yxz(pocket.x + 0.1f * rl,
                                          pocket.y - 0.1f * rl + 0.012f * breathe,
                                          pocket.z - 0.11f * gPlayer.kick - 0.08f * rl,
                                          -0.16f * gPlayer.kick - 0.35f * rl + 0.01f * sinf(gPlayer.t * 1.3f),
                                          0.15f * rl,
                                          0.65f * rl);
    gPlayer.mGun = MatrixMultiply(mGunLocal, gPlayer.mRoot);
    gPlayer.muzzleWorldPos = xform_pt((Vector3){ 0.0f, 0.03f, 1.47f }, gPlayer.mGun);

    // Magazine
    if (!gState.reloading || u < 0.12f) gPlayer.magDropped = false;
    float magDrop = 0.0f;
    if (gState.reloading && u >= 0.12f && u < 0.22f) magDrop = 0.24f * ease_smooth((u - 0.12f) / 0.1f);
    if (gState.reloading && u >= 0.78f) magDrop = 0.2f * (1.0f - ease_smooth(fminf(1.0f, (u - 0.78f) / 0.17f)));
    gPlayer.magVisible = !gState.reloading || u < 0.22f || u >= 0.78f;
    Matrix mMagLocal = make_transform_yxz(0.0f, -0.14f - magDrop, 0.52f, 0.0f, 0.0f, 0.0f);
    gPlayer.mMag = MatrixMultiply(mMagLocal, gPlayer.mGun);
    if (gState.reloading && u >= 0.22f && !gPlayer.magDropped) {
        gPlayer.magDropped = true;
        Vector3 mp = { gPlayer.mMag.m12, gPlayer.mMag.m13, gPlayer.mMag.m14 };
        spawn_debris(DEBRIS_MAG, mp, (Vector3){ 0, -1.0f, 0 },
                     (Vector3){ 0, gPlayer.rotY, 0 }, (Vector3){ 3.0f, 0.0f, 2.0f },
                     (Vector3){ 0.09f, 0.24f, 0.12f }, 4.0f, 0.05f);
    }

    // Arms (two-bone IK)
    Vector3 gR = xform_pt(GRIP_R, mGunLocal);
    Vector3 gL = xform_pt(GRIP_L, mGunLocal);
    if (gPlayer.reload > 0.001f) {
        Vector3 rHand = reload_hand(u, mGunLocal, mUpperLocal);
        gL = Vector3Lerp(gL, rHand, rl);
    }

    const Vector3 poles[2] = { { 0.5f, -1.0f, -0.1f }, { -0.8f, -1.0f, -0.5f } };
    for (int i = 0; i < 2; i++) {
        float side = (i == 0) ? 1.0f : -1.0f;
        Vector3 sh = xform_pt((Vector3){ side * 0.41f, 0.7f, 0.0f }, mUpperLocal);
        Vector3 target = (side < 0.0f) ? gR : gL;
        Vector3 elbow = ik2(sh, target, P_UPPER_ARM, P_FOREARM, poles[i]);
        Vector3 wrist = Vector3Add(elbow, Vector3Scale(Vector3Normalize(Vector3Subtract(target, elbow)), P_FOREARM));
        gPlayer.mUpperArm[i] = MatrixMultiply(bone_transform(sh, elbow, poles[i]), gPlayer.mRoot);
        gPlayer.mForearm[i]  = MatrixMultiply(bone_transform(elbow, wrist, poles[i]), gPlayer.mRoot);
        // Hand aligns with gun orientation at wrist
        Matrix mh = gPlayer.mGun;
        Vector3 wWorld = xform_pt(wrist, gPlayer.mRoot);
        mh.m12 = wWorld.x; mh.m13 = wWorld.y; mh.m14 = wWorld.z;
        gPlayer.mHand[i] = mh;
    }

    gPlayer.spareMagVisible = gState.reloading && u >= 0.45f && u < 0.78f;
    if (gPlayer.spareMagVisible) {
        Matrix sm = gPlayer.mHand[0];
        Vector3 upOffset = { sm.m4 * 0.13f, sm.m5 * 0.13f, sm.m6 * 0.13f };
        sm.m12 += upOffset.x; sm.m13 += upOffset.y; sm.m14 += upOffset.z;
        gPlayer.mSpareMag = sm;
    }

    // Muzzle flash fade
    gPlayer.flash = fmaxf(0.0f, gPlayer.flash - dt / 0.07f);
    float f = gPlayer.flash;
    gPlayer.muzzleLightInt = gPlayer.flashPeak * f * f;
    gPlayer.muzzleLightPos = (Vector3){
        gPlayer.muzzleWorldPos.x + sinf(aim) * 0.4f,
        gPlayer.muzzleWorldPos.y,
        gPlayer.muzzleWorldPos.z + cosf(aim) * 0.4f
    };
}

static void draw_mesh_xform(Model *m, Matrix xform, Color tint) {
    DrawMesh(m->meshes[0], m->materials[0], xform);
    (void)tint;
}

void player_draw(void) {
    if (!gPlayer.visible) return;

    draw_mesh_xform(&gPlayer.pelvisModel, gPlayer.mPelvis, WHITE);
    for (int i = 0; i < 2; i++) {
        draw_mesh_xform(&gPlayer.thighModel, gPlayer.mThigh[i], WHITE);
        draw_mesh_xform(&gPlayer.shinModel,  gPlayer.mShin[i], WHITE);
        draw_mesh_xform(&gPlayer.bootModel,  gPlayer.mBoot[i], WHITE);
    }

    draw_mesh_xform(&gPlayer.upperGearModel, gPlayer.mUpper, WHITE);
    draw_mesh_xform(&gPlayer.headModel,      gPlayer.mHead, WHITE);
    draw_mesh_xform(&gPlayer.gunModel,       gPlayer.mGun, WHITE);
    if (gPlayer.magVisible) draw_mesh_xform(&gPlayer.magModel, gPlayer.mMag, WHITE);
    if (gPlayer.spareMagVisible) draw_mesh_xform(&gPlayer.magModel, gPlayer.mSpareMag, WHITE);
    for (int i = 0; i < 2; i++) {
        draw_mesh_xform(&gPlayer.handModel, gPlayer.mHand[i], WHITE);
    }

    // Flannel parts with plaid texture
    int useTex = 1;
    SetShaderValue(gScene.litShader, gScene.locUseTex, &useTex, SHADER_UNIFORM_INT);
    draw_mesh_xform(&gPlayer.upperFlannelModel, gPlayer.mUpper, WHITE);
    for (int i = 0; i < 2; i++) {
        draw_mesh_xform(&gPlayer.upperArmModel, gPlayer.mUpperArm[i], WHITE);
        draw_mesh_xform(&gPlayer.forearmModel,  gPlayer.mForearm[i], WHITE);
    }
    useTex = 0;
    SetShaderValue(gScene.litShader, gScene.locUseTex, &useTex, SHADER_UNIFORM_INT);

    // Muzzle flash star (unlit additive)
    if (gPlayer.flash > 0.3f) {
        rlDrawRenderBatchActive();
        BeginBlendMode(BLEND_ADDITIVE);
        Vector3 p = gPlayer.muzzleWorldPos;
        float s = gPlayer.flashScale * gPlayer.flash;
        DrawSphere(p, 0.16f * s, (Color){ 255, 243, 208, 220 });
        Vector3 fwd = { gPlayer.mGun.m8, gPlayer.mGun.m9, gPlayer.mGun.m10 };
        Vector3 tip = Vector3Add(p, Vector3Scale(fwd, 0.7f * s));
        DrawCylinderEx(p, tip, 0.14f * s, 0.0f, 4, gPlayer.flashColor);
        EndBlendMode();
    }
}

void player_cleanup(void) {
    UnloadTexture(gPlayer.plaidTex);
    UnloadModel(gPlayer.pelvisModel);
    UnloadModel(gPlayer.thighModel);
    UnloadModel(gPlayer.shinModel);
    UnloadModel(gPlayer.bootModel);
    UnloadModel(gPlayer.upperFlannelModel);
    UnloadModel(gPlayer.upperGearModel);
    UnloadModel(gPlayer.headModel);
    UnloadModel(gPlayer.gunModel);
    UnloadModel(gPlayer.magModel);
    UnloadModel(gPlayer.upperArmModel);
    UnloadModel(gPlayer.forearmModel);
    UnloadModel(gPlayer.handModel);
}
