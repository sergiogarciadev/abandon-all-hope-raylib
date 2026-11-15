# Abandon All Hope — Programming a 3D Game in C with Raylib

Welcome to the companion book for the C + Raylib port of **Abandon All Hope**, a 3D top-down zombie survival shooter originally written in JavaScript and Three.js.

This book walks through the entire port **step by step**, as a practical guide to building a complete 3D game in C11 using **Raylib**, **CMake**, and **LLVM (Clang + LLD)**.

## Table of Contents

1. [How to Use Raylib (Setup, CMake & LLVM)](./01-how-to-use-raylib.md)
2. [Creating an Empty World](./02-creating-an-empty-world.md)
3. [Adding Models (Procedural Chamfered Blocks & Environment)](./03-adding-models.md)
4. [Animating the Models (Procedural Gait & Two-Bone IK)](./04-animating-the-models.md)
5. [Aim and Fire the Gun](./05-aim-and-fire.md)
6. [Zombie AI and Pathfinding](./06-zombie-ai-and-pathfinding.md)
7. [Weather, Effects, and Supply Drops](./07-weather-effects-and-drops.md)
8. [Generating Procedural Audio](./08-procedural-audio.md)
9. [Creating the UI](./09-creating-the-ui.md)

## Quick Build

```bash
cmake -B build -G Ninja -DCMAKE_C_COMPILER=clang -DCMAKE_EXE_LINKER_FLAGS="-fuse-ld=lld"
cmake --build build
./build/abandon_all_hope
```


## Building Lesson Examples

Each chapter includes standalone minimal examples in `examples/`. You can build all of them or a single example via CMake:

```bash
# Build all 12 lesson examples alongside the game
cmake -B build -DBUILD_LESSON_EXAMPLES=ON && cmake --build build

# Or build just one specific example
cmake -B build -DBUILD_LESSON_EXAMPLE=ex04_two_bone_ik && cmake --build build --target ex04_two_bone_ik
```

## Web Build (WebAssembly + WebGL)

Using the [Emscripten SDK](https://emscripten.org/) (`emcmake`):

```bash
emcmake cmake -B build-web -DTARGET_WEB=ON
cmake --build build-web
python3 -m http.server -d build-web 8080
```

Then open `http://localhost:8080/abandon_all_hope.html` in your browser. Setting `TARGET_WEB=ON` (or running under `emcmake`) automatically configures Raylib with `PLATFORM=Web` (WebGL / GLSL ES 1.00), switches `main.c` to `emscripten_set_main_loop()`, and preloads `assets/` into the virtual filesystem.
