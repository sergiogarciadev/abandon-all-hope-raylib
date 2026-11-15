cmake_minimum_required(VERSION 3.20)
project(abandon_all_hope C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Fetch Raylib 5.5
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

add_executable(abandon_all_hope
  src/main.c
  src/core/scene.c
  src/core/blocks.c
  src/core/state.c
  src/world/solids.c
  src/world/openings.c
  src/world/world.c
  src/effects/particles.c
  src/entities/animation.c
  src/entities/player.c
  src/entities/weapon.c
  src/entities/zombies.c
)

target_include_directories(abandon_all_hope PRIVATE src)
target_link_libraries(abandon_all_hope PRIVATE raylib m)

if(CMAKE_C_COMPILER_ID MATCHES "Clang")
  target_compile_options(abandon_all_hope PRIVATE -Wall -Wextra -Wno-missing-braces)
endif()
