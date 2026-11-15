// Lesson 3b: Waypoint Dijkstra routing around rectangular obstacles
#include "raylib.h"
#include <float.h>
#include <math.h>
#include <stdbool.h>

#define BOX_COUNT 3
#define NODE_COUNT (BOX_COUNT * 4)

static Rectangle gBoxes[BOX_COUNT] = {
    { 300, 160, 140, 180 },
    { 540, 100, 120, 140 },
    { 520, 310, 180, 110 }
};

static bool seg_hits_rect(Vector2 a, Vector2 b, Rectangle r) {
    for (int i = 0; i <= 16; i++) {
        float t = (float)i / 16.0f;
        Vector2 p = { a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t };
        if (CheckCollisionPointRec(p, r)) return true;
    }
    return false;
}

static bool clear_line(Vector2 a, Vector2 b) {
    for (int i = 0; i < BOX_COUNT; i++) {
        if (seg_hits_rect(a, b, gBoxes[i])) return false;
    }
    return true;
}

int main(void) {
    InitWindow(960, 540, "Ex 03b: Dijkstra Waypoint Routing Around Solids");
    SetTargetFPS(60);

    Vector2 nodes[NODE_COUNT];
    int n = 0;
    float pad = 18.0f;
    for (int i = 0; i < BOX_COUNT; i++) {
        Rectangle r = gBoxes[i];
        nodes[n++] = (Vector2){ r.x - pad, r.y - pad };
        nodes[n++] = (Vector2){ r.x + r.width + pad, r.y - pad };
        nodes[n++] = (Vector2){ r.x + r.width + pad, r.y + r.height + pad };
        nodes[n++] = (Vector2){ r.x - pad, r.y + r.height + pad };
    }

    Vector2 start = { 120, 270 };
    Vector2 goal = { 820, 270 };

    while (!WindowShouldClose()) {
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) goal = GetMousePosition();
        if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) start = GetMousePosition();

        // Dijkstra over [start, nodes..., goal] -> total NODE_COUNT + 2 vertices
        int total = NODE_COUNT + 2;
        Vector2 pts[NODE_COUNT + 2];
        pts[0] = start;
        for (int i = 0; i < NODE_COUNT; i++) pts[i + 1] = nodes[i];
        pts[total - 1] = goal;

        float dist[NODE_COUNT + 2];
        int prev[NODE_COUNT + 2];
        bool vis[NODE_COUNT + 2] = {0};
        for (int i = 0; i < total; i++) { dist[i] = FLT_MAX; prev[i] = -1; }
        dist[0] = 0.0f;

        for (int iter = 0; iter < total; iter++) {
            int u = -1;
            for (int i = 0; i < total; i++) {
                if (!vis[i] && (u < 0 || dist[i] < dist[u])) u = i;
            }
            if (u < 0 || dist[u] == FLT_MAX) break;
            vis[u] = true;
            for (int v = 0; v < total; v++) {
                if (!vis[v] && clear_line(pts[u], pts[v])) {
                    float nd = dist[u] + hypotf(pts[v].x - pts[u].x, pts[v].y - pts[u].y);
                    if (nd < dist[v]) { dist[v] = nd; prev[v] = u; }
                }
            }
        }

        BeginDrawing();
        ClearBackground((Color){ 14, 16, 22, 255 });
        for (int i = 0; i < BOX_COUNT; i++) DrawRectangleRec(gBoxes[i], (Color){ 72, 62, 54, 255 });
        for (int i = 0; i < NODE_COUNT; i++) DrawCircleV(nodes[i], 4, GRAY);

        int cur = total - 1;
        while (prev[cur] >= 0) {
            DrawLineEx(pts[cur], pts[prev[cur]], 3.0f, (Color){ 232, 210, 138, 255 });
            cur = prev[cur];
        }
        DrawCircleV(start, 9, GREEN);
        DrawCircleV(goal, 9, RED);
        DrawText("Left-click: move Goal (Red) | Right-click: move Start (Green)", 20, 20, 18, RAYWHITE);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
