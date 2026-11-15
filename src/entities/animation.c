#include "animation.h"
#include "core/math_util.h"
#include "raymath.h"
#include <math.h>

FootPose foot_cycle(float p, float beta) {
    FootPose f;
    if (p < beta) {
        f.s = 0.5f - p / beta;
        f.lift = 0.0f;
        f.pitch = 0.0f;
    } else {
        float t = (p - beta) / (1.0f - beta);
        f.s = -0.5f + ease_smooth(t);
        f.lift = sinf(PI * t);
        f.pitch = 0.6f * (1.0f - t) * (1.0f - t) - 0.3f * t * t;
    }
    return f;
}

Vector3 ik2(Vector3 a, Vector3 t, float l1, float l2, Vector3 pole) {
    Vector3 dVec = Vector3Subtract(t, a);
    float rawD = Vector3Length(dVec);
    float d = clampf(rawD, fabsf(l1 - l2) + 1e-3f, l1 + l2 - 1e-3f);
    Vector3 dir = (rawD > 1e-6f) ? Vector3Scale(dVec, 1.0f / rawD) : (Vector3){ 0, -1, 0 };
    float x = (l1 * l1 - l2 * l2 + d * d) / (2.0f * d);
    float dot = Vector3DotProduct(pole, dir);
    Vector3 pProj = Vector3Subtract(pole, Vector3Scale(dir, dot));
    float pLen = Vector3Length(pProj);
    Vector3 pNorm = (pLen > 1e-6f) ? Vector3Scale(pProj, 1.0f / pLen) : (Vector3){ 0, 0, 1 };
    float h = sqrtf(fmaxf(0.0f, l1 * l1 - x * x));
    return Vector3Add(Vector3Add(a, Vector3Scale(dir, x)), Vector3Scale(pNorm, h));
}

Matrix bone_transform(Vector3 a, Vector3 b, Vector3 face) {
    Vector3 by = Vector3Normalize(Vector3Subtract(a, b));
    float dot = Vector3DotProduct(face, by);
    Vector3 bz = Vector3Subtract(face, Vector3Scale(by, dot));
    if (Vector3LengthSqr(bz) < 1e-6f) bz = (Vector3){ 0, 0, 1 };
    else bz = Vector3Normalize(bz);
    Vector3 bx = Vector3CrossProduct(by, bz);
    Vector3 mid = Vector3Scale(Vector3Add(a, b), 0.5f);

    // Column-major Raylib Matrix with basis (bx, by, bz) and translation mid
    Matrix m = {
        bx.x, by.x, bz.x, mid.x,
        bx.y, by.y, bz.y, mid.y,
        bx.z, by.z, bz.z, mid.z,
        0.0f, 0.0f, 0.0f, 1.0f
    };
    return m;
}
