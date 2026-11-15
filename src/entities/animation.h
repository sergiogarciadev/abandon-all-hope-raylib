#ifndef ANIMATION_H
#define ANIMATION_H

#include "raylib.h"

typedef struct {
    float s;
    float lift;
    float pitch;
} FootPose;

FootPose foot_cycle(float p, float beta);
Vector3 ik2(Vector3 a, Vector3 t, float l1, float l2, Vector3 pole);
Matrix bone_transform(Vector3 a, Vector3 b, Vector3 face);

#endif // ANIMATION_H
