#include "raylib.h"
#include "game.h"
#include "ui/title.h"
#include <math.h>
#if defined(PLATFORM_WEB)
#include <emscripten/emscripten.h>
#endif

static void update_draw_frame(void) {
    float dt = fminf(GetFrameTime(), 0.05f);
    game_update(dt);
    game_draw();
}

int main(void) {
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    InitWindow(1280, 720, "Abandon All Hope");
    SetExitKey(KEY_NULL); // ESC pauses gameplay instead of closing the window
    SetTargetFPS(60);

    game_init();

#if defined(PLATFORM_WEB)
    emscripten_set_main_loop(update_draw_frame, 0, 1);
#else
    while (!WindowShouldClose() && !title_should_quit()) {
        update_draw_frame();
    }
#endif

    game_cleanup();
    CloseWindow();
    return 0;
}
