#include "input.h"
#include <string.h>

#if defined(PLATFORM_WEB)
#include <emscripten/emscripten.h>
#include <emscripten/html5.h>

#define MAX_WEB_KEYS 512
#define MAX_WEB_BTNS 8

static bool sKeyDown[MAX_WEB_KEYS];
static bool sKeyPressed[MAX_WEB_KEYS];
static bool sMouseDown[MAX_WEB_BTNS];
static bool sMousePressed[MAX_WEB_BTNS];
static Vector2 sMousePos = { 0.0f, 0.0f };
static bool sMouseSeen = false;

static int map_dom_code(const char *code) {
    if (!code) return 0;
    if (strcmp(code, "KeyW") == 0) return KEY_W;
    if (strcmp(code, "KeyA") == 0) return KEY_A;
    if (strcmp(code, "KeyS") == 0) return KEY_S;
    if (strcmp(code, "KeyD") == 0) return KEY_D;
    if (strcmp(code, "KeyR") == 0) return KEY_R;
    if (strcmp(code, "KeyM") == 0) return KEY_M;
    if (strcmp(code, "KeyP") == 0) return KEY_P;
    if (strcmp(code, "ArrowUp") == 0) return KEY_UP;
    if (strcmp(code, "ArrowDown") == 0) return KEY_DOWN;
    if (strcmp(code, "ArrowLeft") == 0) return KEY_LEFT;
    if (strcmp(code, "ArrowRight") == 0) return KEY_RIGHT;
    if (strcmp(code, "Escape") == 0) return KEY_ESCAPE;
    if (strcmp(code, "Enter") == 0 || strcmp(code, "NumpadEnter") == 0) return KEY_ENTER;
    if (strcmp(code, "Space") == 0) return KEY_SPACE;
    if (strcmp(code, "Digit1") == 0) return KEY_ONE;
    if (strcmp(code, "Digit2") == 0) return KEY_TWO;
    if (strcmp(code, "Digit3") == 0) return KEY_THREE;
    return 0;
}

static EM_BOOL on_web_key(int eventType, const EmscriptenKeyboardEvent *e, void *userData) {
    (void)userData;
    int key = map_dom_code(e->code);
    if (key > 0 && key < MAX_WEB_KEYS) {
        if (eventType == EMSCRIPTEN_EVENT_KEYDOWN) {
            if (!sKeyDown[key]) sKeyPressed[key] = true;
            sKeyDown[key] = true;
        } else if (eventType == EMSCRIPTEN_EVENT_KEYUP) {
            sKeyDown[key] = false;
        }
        return EM_TRUE;
    }
    return EM_FALSE;
}

static void update_web_mouse_pos(const EmscriptenMouseEvent *e) {
    double cw = EM_ASM_DOUBLE({
        var c = document.getElementById('canvas');
        return c ? c.clientWidth : window.innerWidth;
    });
    double ch = EM_ASM_DOUBLE({
        var c = document.getElementById('canvas');
        return c ? c.clientHeight : window.innerHeight;
    });
    float sx = cw > 0.0 ? (float)GetScreenWidth() / (float)cw : 1.0f;
    float sy = ch > 0.0 ? (float)GetScreenHeight() / (float)ch : 1.0f;
    sMousePos.x = (float)e->targetX * sx;
    sMousePos.y = (float)e->targetY * sy;
    sMouseSeen = true;
}

static EM_BOOL on_web_mouse(int eventType, const EmscriptenMouseEvent *e, void *userData) {
    (void)userData;
    update_web_mouse_pos(e);
    int btn = e->button;
    if (btn >= 0 && btn < MAX_WEB_BTNS) {
        if (eventType == EMSCRIPTEN_EVENT_MOUSEDOWN) {
            if (!sMouseDown[btn]) sMousePressed[btn] = true;
            sMouseDown[btn] = true;
        } else if (eventType == EMSCRIPTEN_EVENT_MOUSEUP) {
            sMouseDown[btn] = false;
        }
    }
    return EM_FALSE;
}

void input_init(void) {
    memset(sKeyDown, 0, sizeof(sKeyDown));
    memset(sKeyPressed, 0, sizeof(sKeyPressed));
    memset(sMouseDown, 0, sizeof(sMouseDown));
    memset(sMousePressed, 0, sizeof(sMousePressed));
    emscripten_set_keydown_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, NULL, 1, on_web_key);
    emscripten_set_keyup_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, NULL, 1, on_web_key);
    emscripten_set_mousemove_callback("#canvas", NULL, 1, on_web_mouse);
    emscripten_set_mousedown_callback("#canvas", NULL, 1, on_web_mouse);
    emscripten_set_mouseup_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, NULL, 1, on_web_mouse);
}

void input_end_frame(void) {
    memset(sKeyPressed, 0, sizeof(sKeyPressed));
    memset(sMousePressed, 0, sizeof(sMousePressed));
}

bool input_key_down(int key) {
    return IsKeyDown(key) || (key >= 0 && key < MAX_WEB_KEYS && sKeyDown[key]);
}

bool input_key_pressed(int key) {
    return IsKeyPressed(key) || (key >= 0 && key < MAX_WEB_KEYS && sKeyPressed[key]);
}

Vector2 input_mouse_pos(void) {
    return sMouseSeen ? sMousePos : GetMousePosition();
}

bool input_mouse_down(int button) {
    return IsMouseButtonDown(button) || (button >= 0 && button < MAX_WEB_BTNS && sMouseDown[button]);
}

bool input_mouse_pressed(int button) {
    return IsMouseButtonPressed(button) || (button >= 0 && button < MAX_WEB_BTNS && sMousePressed[button]);
}

#else

void input_init(void) {}
void input_end_frame(void) {}
bool input_key_down(int key) { return IsKeyDown(key); }
bool input_key_pressed(int key) { return IsKeyPressed(key); }
Vector2 input_mouse_pos(void) { return GetMousePosition(); }
bool input_mouse_down(int button) { return IsMouseButtonDown(button); }
bool input_mouse_pressed(int button) { return IsMouseButtonPressed(button); }

#endif
