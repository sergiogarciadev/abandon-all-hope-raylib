#ifndef SCENE_H
#define SCENE_H

#include "raylib.h"

#define CAM_OFFSET_X 0.0f
#define CAM_OFFSET_Y 30.0f
#define CAM_OFFSET_Z 13.0f

typedef struct {
    Camera3D camera;
    Shader litShader;

    // Shader uniform locations
    int locViewPos;
    int locMoonDir;
    int locMoonColor;
    int locAmbientColor;
    int locFogColor;
    int locFogNear;
    int locFogFar;
    int locEmissive;
    int locUseTex;

    // Point lights (0 = porch lantern, 1 = muzzle flash)
    int locLightPos[2];
    int locLightColor[2];
    int locLightIntensity[2];
    int locLightRange[2];

    Color fogColor;
    float fogNear;
    float fogFar;
    float moonIntensity;
    float ambientIntensity;
} GameScene;

extern GameScene gScene;

void scene_init(void);
void scene_cleanup(void);
void scene_update_lighting(Vector3 lanternPos, float lanternInt,
                           Vector3 muzzlePos, float muzzleInt, Color muzzleCol);

#endif // SCENE_H
