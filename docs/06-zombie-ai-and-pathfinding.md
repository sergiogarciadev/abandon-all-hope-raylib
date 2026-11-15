# Chapter 6: Zombie AI and Pathfinding

## 6.1 Four Zombie Kits & Procedural Personality (`zombies.c`)

To keep 30+ zombies cheap to render, we bake 4 distinct local archetypes (`ZOMBIE_KITS[4]`):
- **Farmhand** (denim shirt)
- **Hunter** (field jacket with exposed ribs)
- **Tank Top** (bald, torn scalp, one bare foot)
- **Camper** (mustard hoodie with exposed ribs)

Each spawned zombie gets randomized gait personality: `pace`, `limp` (45% chance of dragging one leg), `hunch`, `tilt`, `arms` (0, 1, or 2 reaching arms), and `turn` rate.

## 6.2 Cabin Entry State Machine (`zombies_chase`)

Zombies spawn offscreen outside the cabin (`zombie_spawn_point()`) and follow a clear state machine:
1. **Melee**: If within `Z_REACH` (`1.2`) and `wall_between()` is false, stop, square up (`Z_TRACK`), wind up for `Z_STRIKE` (`0.2s`), and swing (`Z_SWING = 0.85s`).
2. **Outside the cabin**: Pick `nearest_opening()`, walk to its outer waypoint (`ox, oz`) so it approaches square-on (`stage = 0 -> 1`).
   - If the opening is intact (`hp > 0`), stand at (`fx, fz`) and batter it down (`hit_opening()`).
   - If it's a broken door (`hp <= 0`), walk straight through (`ix, iz`).
   - If it's a broken window and nobody else is climbing it (`climberIdx < 0`), clamber over the sill (`climb_step()`).
3. **Stuck recovery**: If a zombie makes `< 0.6` units of progress over 2 seconds while outside, it abandons its jammed opening and picks the next-nearest one.

## 6.3 Obstacle Avoidance & Dijkstra Visibility Graph (`solids.c`)

For solid yard props like the rusted pickup truck, we precompute a **visibility graph** over the 4 padded corners of each oriented bounding box (`init_solids_nav()`):
- `clear_path(ax, az, bx, bz)` runs a slab-intersection test (`seg_hits_box`) in prop-local space.
- `route_around(pos, tx, tz, &via)` returns `(tx, tz)` directly if unobstructed; otherwise it runs Dijkstra from the goal across the corner nodes and returns the best visible corner (with a `1.0` hysteresis bonus for `via` so zombies don't dither between two equal routes).

Finally, `zombies_separate()` pushes overlapping zombies apart (`Z_SPACE = 0.8`), keeps them outside `Z_CLOSE` (`0.95`) of the survivor, and calls `push_out_of_solids()`.


---

## Hands-On Example: [`ex06_zombie_ai.c`](../examples/ex06_zombie_ai.c)

```bash
cmake -B build -DBUILD_LESSON_EXAMPLE=ex06_zombie_ai && cmake --build build --target ex06_zombie_ai
./build/ex06_zombie_ai
```

Demonstrates the finite-state machine used in `src/entities/zombies.c` (`AI_CHASE` → `AI_BASH` → `AI_CLIMB`):

```c
} else if (st == AI_BASH) {
    timer += dt;
    if (timer >= 0.7f) {
        timer = 0.0f;
        winHp--;
        if (winHp <= 0) st = AI_CLIMB;
    }
} else if (st == AI_CLIMB) {
    timer += dt;
    float u = fminf(1.0f, timer / 1.2f);
    zPos.z = 1.0f - 2.2f * u;
    zPos.y = sinf(u * PI) * 1.15f; // Arc over window sill
    if (u >= 1.0f) { zPos.y = 0.0f; st = AI_CHASE; }
}
```
Notice how `sinf(u * PI)` smoothly lifts the zombie over the window sill before dropping it back to `y = 0` inside the cabin to resume `AI_CHASE`. Press `[R]` to reset the window and zombie.
