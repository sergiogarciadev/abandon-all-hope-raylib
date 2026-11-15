// Lesson 2: Empty 3D world with top-down camera, GLSL 330 moonlight + fog shader
#include "raylib.h"
#include "raymath.h"
#include <math.h>
#include <stddef.h>

static const char *VS =
    "#version 330\n"
    "in vec3 vertexPosition;\n"
    "in vec3 vertexNormal;\n"
    "in vec4 vertexColor;\n"
    "uniform mat4 mvp;\n"
    "uniform mat4 matModel;\n"
    "uniform mat4 matNormal;\n"
    "out vec3 fragPos;\n"
    "out vec3 fragNormal;\n"
    "out vec4 fragColor;\n"
    "void main() {\n"
    "    fragPos = vec3(matModel * vec4(vertexPosition, 1.0));\n"
    "    fragNormal = normalize(vec3(matNormal * vec4(vertexNormal, 0.0)));\n"
    "    fragColor = vertexColor;\n"
    "    gl_Position = mvp * vec4(vertexPosition, 1.0);\n"
    "}\n";

static const char *FS =
    "#version 330\n"
    "in vec3 fragPos;\n"
    "in vec3 fragNormal;\n"
    "in vec4 fragColor;\n"
    "uniform vec3 viewPos;\n"
    "uniform vec3 lightPos;\n"
    "uniform float fogNear;\n"
    "uniform float fogFar;\n"
    "uniform vec3 fogColor;\n"
    "out vec4 finalColor;\n"
    "void main() {\n"
    "    vec3 N = normalize(fragNormal);\n"
    "    vec3 moonDir = normalize(vec3(-0.35, 0.85, -0.4));\n"
    "    float diff = max(dot(N, moonDir), 0.0) * 0.55;\n"
    "    vec3 toL = lightPos - fragPos;\n"
    "    float d = length(toL);\n"
    "    float att = 1.0 / (1.0 + 0.08 * d * d);\n"
    "    float pDiff = max(dot(N, normalize(toL)), 0.0) * att * 2.2;\n"
    "    vec3 lit = fragColor.rgb * (vec3(0.14, 0.17, 0.24) + vec3(0.45, 0.55, 0.78) * diff + vec3(1.0, 0.65, 0.3) * pDiff);\n"
    "    float fog = clamp((length(viewPos - fragPos) - fogNear) / (fogFar - fogNear), 0.0, 1.0);\n"
    "    finalColor = vec4(mix(lit, fogColor, fog), fragColor.a);\n"
    "}\n";

int main(void) {
    InitWindow(960, 540, "Ex 02: Empty World, Moonlight & Fog Shader");
    SetTargetFPS(60);

    Shader sh = LoadShaderFromMemory(VS, FS);
    int locView = GetShaderLocation(sh, "viewPos");
    int locLight = GetShaderLocation(sh, "lightPos");
    int locNear = GetShaderLocation(sh, "fogNear");
    int locFar = GetShaderLocation(sh, "fogFar");
    int locFog = GetShaderLocation(sh, "fogColor");

    float fNear = 24.0f, fFar = 52.0f;
    Vector3 fCol = { 5.0f / 255.0f, 6.0f / 255.0f, 10.0f / 255.0f };
    SetShaderValue(sh, locNear, &fNear, SHADER_UNIFORM_FLOAT);
    SetShaderValue(sh, locFar, &fFar, SHADER_UNIFORM_FLOAT);
    SetShaderValue(sh, locFog, &fCol, SHADER_UNIFORM_VEC3);

    Mesh planeMesh = GenMeshPlane(80.0f, 80.0f, 1, 1);
    Model plane = LoadModelFromMesh(planeMesh);
    plane.materials[0].shader = sh;

    Mesh cubeMesh = GenMeshCube(1.4f, 1.4f, 1.4f);
    Model cube = LoadModelFromMesh(cubeMesh);
    cube.materials[0].shader = sh;

    Camera3D cam = {
        .position = { 0.0f, 22.0f, 12.0f },
        .target = { 0.0f, 0.0f, 0.0f },
        .up = { 0.0f, 1.0f, 0.0f },
        .fovy = 55.0f,
        .projection = CAMERA_PERSPECTIVE
    };

    float t = 0.0f;
    while (!WindowShouldClose()) {
        t += GetFrameTime();
        Vector3 lightPos = { cosf(t * 1.4f) * 6.0f, 2.2f, sinf(t * 1.4f) * 6.0f };
        SetShaderValue(sh, locView, &cam.position, SHADER_UNIFORM_VEC3);
        SetShaderValue(sh, locLight, &lightPos, SHADER_UNIFORM_VEC3);

        BeginDrawing();
        ClearBackground((Color){ 5, 6, 10, 255 });
        BeginMode3D(cam);
        DrawModel(plane, (Vector3){ 0, 0, 0 }, 1.0f, (Color){ 36, 46, 38, 255 });
        for (int z = -2; z <= 2; z++) {
            for (int x = -2; x <= 2; x++) {
                if (x == 0 && z == 0) continue;
                DrawModel(cube, (Vector3){ x * 4.5f, 0.7f, z * 4.5f }, 1.0f, (Color){ 95, 82, 70, 255 });
            }
        }
        DrawSphere(lightPos, 0.25f, ORANGE);
        EndMode3D();
        DrawText("Orbiting point light + directional moonlight + distance fog", 20, 20, 18, RAYWHITE);
        EndDrawing();
    }

    UnloadModel(cube);
    UnloadModel(plane);
    UnloadShader(sh);
    CloseWindow();
    return 0;
}
