cmake_minimum_required(VERSION 3.20)
project(abandon_all_hope C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(TARGET_WEB "Build for WebAssembly + WebGL (use with emcmake)" OFF)
if(EMSCRIPTEN OR TARGET_WEB)
  set(PLATFORM "Web" CACHE STRING "" FORCE)
  set(CMAKE_EXECUTABLE_SUFFIX ".html")
endif()

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
  src/core/storage.c
  src/world/solids.c
  src/world/openings.c
  src/world/world.c
  src/world/weather.c
  src/effects/particles.c
  src/entities/animation.c
  src/entities/player.c
  src/entities/weapon.c
  src/entities/zombies.c
  src/entities/drops.c
  src/audio/synth.c
  src/audio/sfx.c
  src/audio/music.c
  src/ui/fonts.c
  src/ui/hud.c
  src/ui/upgrades.c
  src/ui/title.c
  src/game.c
)

target_include_directories(abandon_all_hope PRIVATE src)
if(EMSCRIPTEN OR TARGET_WEB)
  target_compile_definitions(abandon_all_hope PRIVATE PLATFORM_WEB)
  target_link_libraries(abandon_all_hope PRIVATE raylib m)
  target_link_options(abandon_all_hope PRIVATE
    -sUSE_GLFW=3
    -sALLOW_MEMORY_GROWTH=1
    -sINITIAL_MEMORY=67108864
    -sSTACK_SIZE=2097152
    -sEXPORTED_RUNTIME_METHODS=HEAPF32
    --preload-file ${CMAKE_SOURCE_DIR}/assets@/assets
  )
else()
  target_link_libraries(abandon_all_hope PRIVATE raylib m pthread)
  add_custom_command(TARGET abandon_all_hope POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:abandon_all_hope>/assets
  )
endif()

# Optional lesson examples:
#   -DBUILD_LESSON_EXAMPLES=ON               (build all examples)
#   -DBUILD_LESSON_EXAMPLE=ex04_two_bone_ik  (build a single example)
option(BUILD_LESSON_EXAMPLES "Build all standalone lesson examples" OFF)
set(BUILD_LESSON_EXAMPLE "" CACHE STRING "Build a single lesson example target (e.g. ex04_two_bone_ik)")

set(LESSON_EXAMPLES
  ex01_window
  ex02_empty_world
  ex03_chamfered_mesh
  ex03_dijkstra_grid
  ex04_gait_cycle
  ex04_two_bone_ik
  ex05_raycast_aim
  ex06_zombie_ai
  ex07_storm_weather
  ex08_procedural_synth
  ex09_pixel_icons_hud
  ex09_distressed_ui
)

foreach(ex IN LISTS LESSON_EXAMPLES)
  if(BUILD_LESSON_EXAMPLES OR BUILD_LESSON_EXAMPLE STREQUAL "${ex}")
    add_executable(${ex} examples/${ex}.c)
    target_link_libraries(${ex} PRIVATE raylib m pthread)
    if(CMAKE_C_COMPILER_ID MATCHES "Clang")
      target_compile_options(${ex} PRIVATE -Wall -Wextra -Wno-missing-braces)
    endif()
  endif()
endforeach()

if(CMAKE_C_COMPILER_ID MATCHES "Clang")
  target_compile_options(abandon_all_hope PRIVATE -Wall -Wextra -Wno-missing-braces)
endif()
