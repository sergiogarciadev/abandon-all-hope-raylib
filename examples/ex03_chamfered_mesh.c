// Lesson 3a: Procedural 45-degree chamfered block mesh with baked vertex colors
#include "raylib.h"
#include "raymath.h"
#include <stdlib.h>

static Mesh make_chamfered_box(float w, float h, float d, float b, Color col) {
    // Simplified 6-quad bevelled prism: outer box with chamfered top/bottom/side edges
    // Built as 26 triangles around an extruded octagon cross-section (or 44 quads -> 88 tris)
    // Here we build a clean chamfered prism (8-sided XY profile extruded along Z, plus beveled caps)
    float hx = w * 0.5f, hy = h * 0.5f, hz = d * 0.5f;
    float ix = hx - b, iy = hy - b, iz = hz - b;

    // 24 corner vertices (3 per octant, pushed inward on one axis by `b`)
    Vector3 corners[24];
    int idx = 0;
    for (int sz = -1; sz <= 1; sz += 2) {
        for (int sy = -1; sy <= 1; sy += 2) {
            for (int sx = -1; sx <= 1; sx += 2) {
                corners[idx++] = (Vector3){ sx * ix, sy * iy, sz * hz }; // Z-face
                corners[idx++] = (Vector3){ sx * hx, sy * iy, sz * iz }; // X-face
                corners[idx++] = (Vector3){ sx * ix, sy * hy, sz * iz }; // Y-face
            }
        }
    }

    // Build mesh using Raylib's Mesh struct (12 bevel tris + 18 quad faces = 48 tris = 144 verts)
    // For clarity in this minimal example, we emit flat-shaded bevel corners + main faces
    const int triCount = 44;
    Mesh m = {0};
    m.vertexCount = triCount * 3;
    m.triangleCount = triCount;
    m.vertices = (float *)MemAlloc((unsigned int)(m.vertexCount * 3) * sizeof(float));
    m.normals  = (float *)MemAlloc((unsigned int)(m.vertexCount * 3) * sizeof(float));
    m.colors   = (unsigned char *)MemAlloc((unsigned int)(m.vertexCount * 4));

    int v = 0;
    #define PUSH_TRI(A, B, C) do { \
        Vector3 pa = (A), pb = (B), pc = (C); \
        Vector3 n = Vector3Normalize(Vector3CrossProduct(Vector3Subtract(pb, pa), Vector3Subtract(pc, pa))); \
        Vector3 pts[3] = { pa, pb, pc }; \
        for (int k = 0; k < 3; k++) { \
            m.vertices[v * 3 + 0] = pts[k].x; m.vertices[v * 3 + 1] = pts[k].y; m.vertices[v * 3 + 2] = pts[k].z; \
            m.normals[v * 3 + 0] = n.x; m.normals[v * 3 + 1] = n.y; m.normals[v * 3 + 2] = n.z; \
            m.colors[v * 4 + 0] = col.r; m.colors[v * 4 + 1] = col.g; m.colors[v * 4 + 2] = col.b; m.colors[v * 4 + 3] = 255; \
            v++; \
        } \
    } while (0)
    #define PUSH_QUAD(A, B, C, D) do { PUSH_TRI(A, B, C); PUSH_TRI(A, C, D); } while (0)

    // 6 main faces
    PUSH_QUAD(((Vector3){-ix,-iy, hz}), ((Vector3){ ix,-iy, hz}), ((Vector3){ ix, iy, hz}), ((Vector3){-ix, iy, hz})); // +Z
    PUSH_QUAD(((Vector3){ ix,-iy,-hz}), ((Vector3){-ix,-iy,-hz}), ((Vector3){-ix, iy,-hz}), ((Vector3){ ix, iy,-hz})); // -Z
    PUSH_QUAD(((Vector3){ hx,-iy, iz}), ((Vector3){ hx,-iy,-iz}), ((Vector3){ hx, iy,-iz}), ((Vector3){ hx, iy, iz})); // +X
    PUSH_QUAD(((Vector3){-hx,-iy,-iz}), ((Vector3){-hx,-iy, iz}), ((Vector3){-hx, iy, iz}), ((Vector3){-hx, iy,-iz})); // -X
    PUSH_QUAD(((Vector3){-ix, hy, iz}), ((Vector3){ ix, hy, iz}), ((Vector3){ ix, hy,-iz}), ((Vector3){-ix, hy,-iz})); // +Y
    PUSH_QUAD(((Vector3){-ix,-hy,-iz}), ((Vector3){ ix,-hy,-iz}), ((Vector3){ ix,-hy, iz}), ((Vector3){-ix,-hy, iz})); // -Y

    // 12 edge chamfers
    PUSH_QUAD(((Vector3){-ix, iy, hz}), ((Vector3){ ix, iy, hz}), ((Vector3){ ix, hy, iz}), ((Vector3){-ix, hy, iz}));
    PUSH_QUAD(((Vector3){ ix,-iy, hz}), ((Vector3){-ix,-iy, hz}), ((Vector3){-ix,-hy, iz}), ((Vector3){ ix,-hy, iz}));
    PUSH_QUAD(((Vector3){ ix, iy,-hz}), ((Vector3){-ix, iy,-hz}), ((Vector3){-ix, hy,-iz}), ((Vector3){ ix, hy,-iz}));
    PUSH_QUAD(((Vector3){-ix,-iy,-hz}), ((Vector3){ ix,-iy,-hz}), ((Vector3){ ix,-hy,-iz}), ((Vector3){-ix,-hy,-iz}));
    PUSH_QUAD(((Vector3){ ix,-iy, hz}), ((Vector3){ hx,-iy, iz}), ((Vector3){ hx, iy, iz}), ((Vector3){ ix, iy, hz}));
    PUSH_QUAD(((Vector3){-hx,-iy, iz}), ((Vector3){-ix,-iy, hz}), ((Vector3){-ix, iy, hz}), ((Vector3){-hx, iy, iz}));
    PUSH_QUAD(((Vector3){ hx,-iy,-iz}), ((Vector3){ ix,-iy,-hz}), ((Vector3){ ix, iy,-hz}), ((Vector3){ hx, iy,-iz}));
    PUSH_QUAD(((Vector3){-ix,-iy,-hz}), ((Vector3){-hx,-iy,-iz}), ((Vector3){-hx, iy,-iz}), ((Vector3){-ix, iy,-hz}));
    PUSH_QUAD(((Vector3){ hx, iy, iz}), ((Vector3){ hx, iy,-iz}), ((Vector3){ ix, hy,-iz}), ((Vector3){ ix, hy, iz}));
    PUSH_QUAD(((Vector3){-hx, iy,-iz}), ((Vector3){-hx, iy, iz}), ((Vector3){-ix, hy, iz}), ((Vector3){-ix, hy,-iz}));
    PUSH_QUAD(((Vector3){ hx,-iy,-iz}), ((Vector3){ hx,-iy, iz}), ((Vector3){ ix,-hy, iz}), ((Vector3){ ix,-hy,-iz}));
    PUSH_QUAD(((Vector3){-hx,-iy, iz}), ((Vector3){-hx,-iy,-iz}), ((Vector3){-ix,-hy,-iz}), ((Vector3){-ix,-hy, iz}));

    // 8 corner triangles
    PUSH_TRI(((Vector3){ ix, iy, hz}), ((Vector3){ hx, iy, iz}), ((Vector3){ ix, hy, iz}));
    PUSH_TRI(((Vector3){-hx, iy, iz}), ((Vector3){-ix, iy, hz}), ((Vector3){-ix, hy, iz}));
    PUSH_TRI(((Vector3){ hx,-iy, iz}), ((Vector3){ ix,-iy, hz}), ((Vector3){ ix,-hy, iz}));
    PUSH_TRI(((Vector3){-ix,-iy, hz}), ((Vector3){-hx,-iy, iz}), ((Vector3){-ix,-hy, iz}));
    PUSH_TRI(((Vector3){ hx, iy,-iz}), ((Vector3){ ix, iy,-hz}), ((Vector3){ ix, hy,-iz}));
    PUSH_TRI(((Vector3){-ix, iy,-hz}), ((Vector3){-hx, iy,-iz}), ((Vector3){-ix, hy,-iz}));
    PUSH_TRI(((Vector3){ ix,-iy,-hz}), ((Vector3){ hx,-iy,-iz}), ((Vector3){ ix,-hy,-iz}));
    PUSH_TRI(((Vector3){-hx,-iy,-iz}), ((Vector3){-ix,-iy,-hz}), ((Vector3){-ix,-hy,-iz}));
    #undef PUSH_QUAD
    #undef PUSH_TRI

    UploadMesh(&m, false);
    return m;
}

int main(void) {
    InitWindow(960, 540, "Ex 03a: Procedural 45-Degree Chamfered Block");
    SetTargetFPS(60);

    Mesh m = make_chamfered_box(2.2f, 1.5f, 1.8f, 0.22f, (Color){ 190, 140, 90, 255 });
    Model model = LoadModelFromMesh(m);

    Camera3D cam = { .position = { 4, 3, 4 }, .target = { 0, 0, 0 }, .up = { 0, 1, 0 }, .fovy = 45, .projection = CAMERA_PERSPECTIVE };
    float angle = 0.0f;

    while (!WindowShouldClose()) {
        angle += GetFrameTime() * 0.8f;
        BeginDrawing();
        ClearBackground((Color){ 16, 18, 24, 255 });
        BeginMode3D(cam);
        DrawModelEx(model, (Vector3){ 0, 0, 0 }, (Vector3){ 0.3f, 1.0f, 0.1f }, angle * RAD2DEG, Vector3One(), WHITE);
        DrawModelWiresEx(model, (Vector3){ 0, 0, 0 }, (Vector3){ 0.3f, 1.0f, 0.1f }, angle * RAD2DEG, Vector3One(), (Color){ 255, 255, 255, 80 });
        EndMode3D();
        DrawText("45-degree chamfered cuboid (44 triangles, wireframe overlay)", 20, 20, 18, RAYWHITE);
        EndDrawing();
    }

    UnloadModel(model);
    CloseWindow();
    return 0;
}
