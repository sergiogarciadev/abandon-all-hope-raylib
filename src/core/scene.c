#include "scene.h"
#include "math_util.h"
#include "rlgl.h"
#include <stddef.h>
#include <stdio.h>

GameScene gScene = {0};

#if defined(PLATFORM_WEB)
static const char *LIT_VS =
    "#version 100\n"
    "attribute vec3 vertexPosition;\n"
    "attribute vec2 vertexTexCoord;\n"
    "attribute vec3 vertexNormal;\n"
    "attribute vec4 vertexColor;\n"
    "uniform mat4 mvp;\n"
    "uniform mat4 matModel;\n"
    "uniform mat4 matNormal;\n"
    "varying vec3 fragPosition;\n"
    "varying vec2 fragTexCoord;\n"
    "varying vec3 fragNormal;\n"
    "varying vec4 fragColor;\n"
    "void main() {\n"
    "    vec4 worldPos = matModel * vec4(vertexPosition, 1.0);\n"
    "    fragPosition = worldPos.xyz;\n"
    "    fragTexCoord = vertexTexCoord;\n"
    "    fragNormal = normalize(vec3(matNormal * vec4(vertexNormal, 0.0)));\n"
    "    fragColor = vertexColor;\n"
    "    gl_Position = mvp * vec4(vertexPosition, 1.0);\n"
    "}\n";

static const char *LIT_FS =
    "#version 100\n"
    "precision mediump float;\n"
    "varying vec3 fragPosition;\n"
    "varying vec2 fragTexCoord;\n"
    "varying vec3 fragNormal;\n"
    "varying vec4 fragColor;\n"
    "uniform sampler2D texture0;\n"
    "uniform vec4 colDiffuse;\n"
    "uniform int useTex;\n"
    "uniform vec3 viewPos;\n"
    "uniform vec3 moonDir;\n"
    "uniform vec3 moonColor;\n"
    "uniform vec3 ambientColor;\n"
    "uniform vec3 fogColor;\n"
    "uniform float fogNear;\n"
    "uniform float fogFar;\n"
    "uniform vec3 emissive;\n"
    "uniform vec3 lightPos[2];\n"
    "uniform vec3 lightColor[2];\n"
    "uniform float lightIntensity[2];\n"
    "uniform float lightRange[2];\n"
    "void main() {\n"
    "    vec4 base = fragColor * colDiffuse;\n"
    "    if (useTex == 1) base *= texture2D(texture0, fragTexCoord);\n"
    "    vec3 N = normalize(fragNormal);\n"
    "    vec3 L = normalize(moonDir);\n"
    "    float diff = max(dot(N, L), 0.0);\n"
    "    vec3 lit = base.rgb * (ambientColor + moonColor * diff);\n"
    "    for (int i = 0; i < 2; i++) {\n"
    "        if (lightIntensity[i] > 0.001) {\n"
    "            vec3 toL = lightPos[i] - fragPosition;\n"
    "            float d = length(toL);\n"
    "            if (d < lightRange[i]) {\n"
    "                vec3 ld = toL / max(d, 0.001);\n"
    "                float ndl = max(dot(N, ld), 0.0);\n"
    "                float att = 1.0 - smoothstep(0.0, lightRange[i], d);\n"
    "                att = att * att;\n"
    "                lit += base.rgb * lightColor[i] * (ndl * att * lightIntensity[i]);\n"
    "            }\n"
    "        }\n"
    "    }\n"
    "    lit += emissive;\n"
    "    float dist = length(viewPos - fragPosition);\n"
    "    float fog = clamp((dist - fogNear) / max(fogFar - fogNear, 0.001), 0.0, 1.0);\n"
    "    vec3 col = mix(lit, fogColor, fog);\n"
    "    gl_FragColor = vec4(col, base.a);\n"
    "}\n";
#else
static const char *LIT_VS =
    "#version 330\n"
    "in vec3 vertexPosition;\n"
    "in vec2 vertexTexCoord;\n"
    "in vec3 vertexNormal;\n"
    "in vec4 vertexColor;\n"
    "uniform mat4 mvp;\n"
    "uniform mat4 matModel;\n"
    "uniform mat4 matNormal;\n"
    "out vec3 fragPosition;\n"
    "out vec2 fragTexCoord;\n"
    "out vec3 fragNormal;\n"
    "out vec4 fragColor;\n"
    "void main() {\n"
    "    vec4 worldPos = matModel * vec4(vertexPosition, 1.0);\n"
    "    fragPosition = worldPos.xyz;\n"
    "    fragTexCoord = vertexTexCoord;\n"
    "    fragNormal = normalize(vec3(matNormal * vec4(vertexNormal, 0.0)));\n"
    "    fragColor = vertexColor;\n"
    "    gl_Position = mvp * vec4(vertexPosition, 1.0);\n"
    "}\n";

static const char *LIT_FS =
    "#version 330\n"
    "in vec3 fragPosition;\n"
    "in vec2 fragTexCoord;\n"
    "in vec3 fragNormal;\n"
    "in vec4 fragColor;\n"
    "uniform sampler2D texture0;\n"
    "uniform vec4 colDiffuse;\n"
    "uniform int useTex;\n"
    "uniform vec3 viewPos;\n"
    "uniform vec3 moonDir;\n"
    "uniform vec3 moonColor;\n"
    "uniform vec3 ambientColor;\n"
    "uniform vec3 fogColor;\n"
    "uniform float fogNear;\n"
    "uniform float fogFar;\n"
    "uniform vec3 emissive;\n"
    "uniform vec3 lightPos[2];\n"
    "uniform vec3 lightColor[2];\n"
    "uniform float lightIntensity[2];\n"
    "uniform float lightRange[2];\n"
    "out vec4 finalColor;\n"
    "void main() {\n"
    "    vec4 base = fragColor * colDiffuse;\n"
    "    if (useTex == 1) base *= texture(texture0, fragTexCoord);\n"
    "    vec3 N = normalize(fragNormal);\n"
    "    vec3 L = normalize(moonDir);\n"
    "    float diff = max(dot(N, L), 0.0);\n"
    "    vec3 lit = base.rgb * (ambientColor + moonColor * diff);\n"
    "    for (int i = 0; i < 2; i++) {\n"
    "        if (lightIntensity[i] > 0.001) {\n"
    "            vec3 toL = lightPos[i] - fragPosition;\n"
    "            float d = length(toL);\n"
    "            if (d < lightRange[i]) {\n"
    "                vec3 ld = toL / max(d, 0.001);\n"
    "                float ndl = max(dot(N, ld), 0.0);\n"
    "                float att = 1.0 - smoothstep(0.0, lightRange[i], d);\n"
    "                att = att * att;\n"
    "                lit += base.rgb * lightColor[i] * (ndl * att * lightIntensity[i]);\n"
    "            }\n"
    "        }\n"
    "    }\n"
    "    lit += emissive;\n"
    "    float dist = length(viewPos - fragPosition);\n"
    "    float fog = clamp((dist - fogNear) / max(fogFar - fogNear, 0.001), 0.0, 1.0);\n"
    "    vec3 col = mix(lit, fogColor, fog);\n"
    "    finalColor = vec4(col, base.a);\n"
    "}\n";
#endif

void scene_init(void) {
    gScene.camera.position = (Vector3){ CAM_OFFSET_X, CAM_OFFSET_Y, CAM_OFFSET_Z };
    gScene.camera.target = (Vector3){ 0.0f, 0.0f, 0.0f };
    gScene.camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    gScene.camera.fovy = 55.0f;
    gScene.camera.projection = CAMERA_PERSPECTIVE;

    gScene.fogColor = (Color){ 5, 6, 10, 255 };
    gScene.fogNear = 32.0f;
    gScene.fogFar = 62.0f;
    gScene.moonIntensity = 1.0f;
    gScene.ambientIntensity = 1.0f;

    gScene.litShader = LoadShaderFromMemory(LIT_VS, LIT_FS);
    gScene.litShader.locs[SHADER_LOC_MATRIX_MODEL] = GetShaderLocation(gScene.litShader, "matModel");
    gScene.litShader.locs[SHADER_LOC_MATRIX_NORMAL] = GetShaderLocation(gScene.litShader, "matNormal");
    gScene.litShader.locs[SHADER_LOC_VECTOR_VIEW] = GetShaderLocation(gScene.litShader, "viewPos");

    gScene.locViewPos = GetShaderLocation(gScene.litShader, "viewPos");
    gScene.locMoonDir = GetShaderLocation(gScene.litShader, "moonDir");
    gScene.locMoonColor = GetShaderLocation(gScene.litShader, "moonColor");
    gScene.locAmbientColor = GetShaderLocation(gScene.litShader, "ambientColor");
    gScene.locFogColor = GetShaderLocation(gScene.litShader, "fogColor");
    gScene.locFogNear = GetShaderLocation(gScene.litShader, "fogNear");
    gScene.locFogFar = GetShaderLocation(gScene.litShader, "fogFar");
    gScene.locEmissive = GetShaderLocation(gScene.litShader, "emissive");
    gScene.locUseTex = GetShaderLocation(gScene.litShader, "useTex");

    for (int i = 0; i < 2; i++) {
        char buf[32];
        snprintf(buf, sizeof(buf), "lightPos[%d]", i);
        gScene.locLightPos[i] = GetShaderLocation(gScene.litShader, buf);
        snprintf(buf, sizeof(buf), "lightColor[%d]", i);
        gScene.locLightColor[i] = GetShaderLocation(gScene.litShader, buf);
        snprintf(buf, sizeof(buf), "lightIntensity[%d]", i);
        gScene.locLightIntensity[i] = GetShaderLocation(gScene.litShader, buf);
        snprintf(buf, sizeof(buf), "lightRange[%d]", i);
        gScene.locLightRange[i] = GetShaderLocation(gScene.litShader, buf);
    }

    Vector3 moonDir = Vector3Normalize((Vector3){ -18.0f, 34.0f, -10.0f });
    SetShaderValue(gScene.litShader, gScene.locMoonDir, &moonDir, SHADER_UNIFORM_VEC3);
    Vector3 zero = { 0.0f, 0.0f, 0.0f };
    SetShaderValue(gScene.litShader, gScene.locEmissive, &zero, SHADER_UNIFORM_VEC3);
    int useTex = 0;
    SetShaderValue(gScene.litShader, gScene.locUseTex, &useTex, SHADER_UNIFORM_INT);
}

void scene_update_lighting(Vector3 lanternPos, float lanternInt,
                           Vector3 muzzlePos, float muzzleInt, Color muzzleCol) {
    float viewPos[3] = { gScene.camera.position.x, gScene.camera.position.y, gScene.camera.position.z };
    SetShaderValue(gScene.litShader, gScene.locViewPos, viewPos, SHADER_UNIFORM_VEC3);

    float mi = gScene.moonIntensity;
    float moonCol[3] = { 0.77f * 0.72f * mi, 0.83f * 0.72f * mi, 0.95f * 0.72f * mi };
    SetShaderValue(gScene.litShader, gScene.locMoonColor, moonCol, SHADER_UNIFORM_VEC3);

    float ai = gScene.ambientIntensity;
    float ambCol[3] = { 0.29f * 0.65f * ai, 0.36f * 0.65f * ai, 0.50f * 0.65f * ai };
    SetShaderValue(gScene.litShader, gScene.locAmbientColor, ambCol, SHADER_UNIFORM_VEC3);

    float fogCol[3] = {
        (float)gScene.fogColor.r / 255.0f,
        (float)gScene.fogColor.g / 255.0f,
        (float)gScene.fogColor.b / 255.0f
    };
    SetShaderValue(gScene.litShader, gScene.locFogColor, fogCol, SHADER_UNIFORM_VEC3);
    SetShaderValue(gScene.litShader, gScene.locFogNear, &gScene.fogNear, SHADER_UNIFORM_FLOAT);
    SetShaderValue(gScene.litShader, gScene.locFogFar, &gScene.fogFar, SHADER_UNIFORM_FLOAT);

    // Light 0: porch lantern
    float l0Pos[3] = { lanternPos.x, lanternPos.y, lanternPos.z };
    float l0Col[3] = { 1.0f, 0.65f, 0.29f };
    float l0Range = 9.5f;
    SetShaderValue(gScene.litShader, gScene.locLightPos[0], l0Pos, SHADER_UNIFORM_VEC3);
    SetShaderValue(gScene.litShader, gScene.locLightColor[0], l0Col, SHADER_UNIFORM_VEC3);
    SetShaderValue(gScene.litShader, gScene.locLightIntensity[0], &lanternInt, SHADER_UNIFORM_FLOAT);
    SetShaderValue(gScene.litShader, gScene.locLightRange[0], &l0Range, SHADER_UNIFORM_FLOAT);

    // Light 1: muzzle flash
    float l1Pos[3] = { muzzlePos.x, muzzlePos.y, muzzlePos.z };
    float l1Col[3] = { (float)muzzleCol.r / 255.0f, (float)muzzleCol.g / 255.0f, (float)muzzleCol.b / 255.0f };
    float l1Range = 14.0f;
    SetShaderValue(gScene.litShader, gScene.locLightPos[1], l1Pos, SHADER_UNIFORM_VEC3);
    SetShaderValue(gScene.litShader, gScene.locLightColor[1], l1Col, SHADER_UNIFORM_VEC3);
    SetShaderValue(gScene.litShader, gScene.locLightIntensity[1], &muzzleInt, SHADER_UNIFORM_FLOAT);
    SetShaderValue(gScene.litShader, gScene.locLightRange[1], &l1Range, SHADER_UNIFORM_FLOAT);
}

void scene_cleanup(void) {
    UnloadShader(gScene.litShader);
}
