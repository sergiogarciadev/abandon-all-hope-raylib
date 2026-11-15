#ifndef PLAYER_H
#define PLAYER_H

#include "raylib.h"
#include <stdbool.h>

typedef struct {
    Vector3 position;
    float rotY;
    bool visible;

    // Rig animation state
    float phase;
    float gait;
    float turnW;
    bool turning;
    float hipYaw;
    bool back;
    Vector3 stride;
    float leanF, leanS;
    float kick;
    float flash;
    float flashPeak;
    Color flashColor;
    float reload;
    bool magDropped;
    float t;

    // Computed world transforms for rendering
    Matrix mRoot;
    Matrix mLower;
    Matrix mPelvis;
    Matrix mUpper;
    Matrix mHead;
    Matrix mGun;
    Matrix mMag;
    Matrix mSpareMag;
    bool magVisible;
    bool spareMagVisible;

    Matrix mThigh[2];
    Matrix mShin[2];
    Matrix mBoot[2];
    Matrix mUpperArm[2];
    Matrix mForearm[2];
    Matrix mHand[2];

    Vector3 muzzleWorldPos;
    Vector3 muzzleLightPos;
    float muzzleLightInt;
    float flashRotZ;
    float flashScale;

    // Models
    Texture2D plaidTex;
    Model pelvisModel;
    Model thighModel;
    Model shinModel;
    Model bootModel;
    Model upperFlannelModel;
    Model upperGearModel;
    Model headModel;
    Model gunModel;
    Model magModel;
    Model upperArmModel;
    Model forearmModel;
    Model handModel;
} PlayerRig;

extern PlayerRig gPlayer;

void player_init(void);
void player_animate(float dt, float mx, float mz);
void player_muzzle_flash(bool hot);
void player_draw(void);
void player_cleanup(void);

#endif // PLAYER_H
