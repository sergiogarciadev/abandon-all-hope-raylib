# Chapter 5: Aim and Fire the Gun

## 5.1 Mouse Raycasting onto the Aim Plane

From a top-down perspective camera, the mouse cursor gives 2D screen coordinates. Raylib's `GetScreenToWorldRay(GetMousePosition(), gScene.camera)` projects a 3D ray from the camera through the cursor. We intersect that ray with the horizontal plane at rifle height (`y = 0.9`):

```c
Ray ray = GetScreenToWorldRay(GetMousePosition(), gScene.camera);
if (fabsf(ray.direction.y) > 1e-5f) {
    float t = (0.9f - ray.position.y) / ray.direction.y;
    if (t > 0.0f) {
        aimPoint.x = ray.position.x + ray.direction.x * t;
        aimPoint.z = ray.position.z + ray.direction.z * t;
        gPlayer.rotY = atan2f(aimPoint.x - gPlayer.position.x,
                              aimPoint.z - gPlayer.position.z);
    }
}
```

## 5.2 Firing, Spread, and Tracers (`weapon.c`)

When firing:
- `weapon_fire()` checks `fireCd`, `reloading`, and `mag > 0`.
- `1 + gState.spread` bullets fan out by `0.13 rad` around the normalized aim vector, starting at `gPlayer.muzzleWorldPos`.
- Each shot triggers `player_muzzle_flash()`, which kicks the rifle back, lights the muzzle point light (`light[1]` in our shader), ejects a spinning brass casing, and puffs gun smoke (`particles.c`).

## 5.3 Sub-Stepped Ballistics

Bullets travel at `38 units/second`. At 30 FPS (`dt = 0.033s`), a single step is `1.25` units—wider than a `0.7`-unit log wall! To prevent tunneling:
- We divide the segment traveled this frame into steps of at most `WALL_T * 0.5`.
- Each sub-step checks `gWorld.wallBoxes` and `shut_at(sx, sz)` (doors stop bullets; glass windows shatter on impact and let the bullet pass through).
- For zombies, we compute the closest point on the bullet's line segment to each zombie center (`dx*dx + dz*dz < 0.55`).


---

## Hands-On Example

- [`examples/ex05_raycast_aim.c`](../examples/ex05_raycast_aim.c) — Mouse raycast aiming onto `y = 0.9`, tracer bullets, and ejecting brass casings (`-DBUILD_LESSON_EXAMPLE=ex05_raycast_aim`).
