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
