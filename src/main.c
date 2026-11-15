#include "raylib.h"
#include "game.h"
#include <math.h>

int main(void) {
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    InitWindow(1280, 720, "Abandon All Hope");
    SetExitKey(KEY_NULL); // ESC pauses gameplay instead of closing the window
    SetTargetFPS(60);

    game_init();

    while (!WindowShouldClose()) {
        float dt = fminf(GetFrameTime(), 0.05f);
        game_update(dt);
        game_draw();
    }

    game_cleanup();
    CloseWindow();
    return 0;
}
