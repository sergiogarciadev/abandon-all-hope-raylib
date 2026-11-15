# Chapter 1: How to Use Raylib

## 1.1 Why C, Raylib, CMake, and LLVM?

Porting a Three.js game to C with **Raylib** strips away browser abstractions and puts you directly in control of memory, rendering, and audio:

- **Raylib** provides a clean, single-header C API (`raylib.h`) for windowing, 3D/2D rendering, input, math (`raymath.h`), and real-time audio streams.
- **CMake** handles cross-platform configuration and automatically fetches Raylib 5.5 via `FetchContent`, so there are no manual library installations.
- **LLVM (`clang` + `lld`)** gives fast compilation, strict warnings (`-Wall -Wextra`), and rapid linking.

## 1.2 Project Configuration (`CMakeLists.txt`)

Instead of relying on a system-installed Raylib package, we declare Raylib 5.5 with `FetchContent`:

```cmake
cmake_minimum_required(VERSION 3.20)
project(abandon_all_hope C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

include(FetchContent)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
  raylib
  GIT_REPOSITORY https://github.com/raysan5/raylib.git
  GIT_TAG 5.5
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(raylib)
```

To configure and build with Clang and LLD:

```bash
cmake -B build -G Ninja -DCMAKE_C_COMPILER=clang -DCMAKE_EXE_LINKER_FLAGS="-fuse-ld=lld"
cmake --build build
```

## 1.3 The Raylib Application Lifecycle

Every Raylib program follows a simple, explicit lifecycle:

1. **Configure flags** (`SetConfigFlags`) *before* creating the window (MSAA 4x, resizable window, VSync).
2. **Initialize** (`InitWindow`, `SetTargetFPS`).
3. **Main Loop** (`while (!WindowShouldClose())`):
   - Update game state using `GetFrameTime()` (`dt`).
   - Render between `BeginDrawing()` and `EndDrawing()`.
4. **Cleanup** (`CloseWindow()`).

```c
#include "raylib.h"

int main(void) {
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    InitWindow(1280, 720, "Abandon All Hope");
    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground((Color){5, 6, 10, 255});
        DrawText("ABANDON ALL HOPE - Raylib + CMake + LLVM", 40, 40, 24, RAYWHITE);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
```

### Key Takeaways

- `BeginDrawing()` / `EndDrawing()` bookend every frame's rendering.
- `GetFrameTime()` returns the elapsed seconds since the previous frame—always clamp it (e.g., `fminf(GetFrameTime(), 0.05f)`) so alt-tabbing or debugging never causes huge physics jumps.
- Raylib uses a **right-handed** 3D coordinate system (`+Y` up, `+X` right, `+Z` toward the viewer), matching Three.js! This makes porting positions, rotations, and camera offsets straightforward.


---

## Hands-On Example: [`ex01_window.c`](../examples/ex01_window.c)

Build and run this standalone example:
```bash
cmake -B build -DBUILD_LESSON_EXAMPLE=ex01_window && cmake --build build --target ex01_window
./build/ex01_window
```

Inside `ex01_window.c`, three core Raylib patterns drive every frame:

1. **Clamped Delta Time** — prevents physics spikes if the window is dragged or pauses:
   ```c
   float dt = fminf(GetFrameTime(), 0.05f);
   ```
2. **Normalized Diagonal Movement** — prevents moving $\sqrt{2}$ faster when holding two keys (`W` + `D`):
   ```c
   float len = hypotf(mx, my);
   if (len > 0.0f) {
       pos.x += (mx / len) * speed * dt;
       pos.y += (my / len) * speed * dt;
   }
   ```
3. **Immediate-Mode Render Loop** — `BeginDrawing()` / `ClearBackground()` / `EndDrawing()` redraws the entire frame at 60 FPS (`SetTargetFPS(60)`).
