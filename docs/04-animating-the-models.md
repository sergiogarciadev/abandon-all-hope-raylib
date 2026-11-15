# Chapter 4: Animating the Models

## 4.1 Procedural Gait & Two-Bone Inverse Kinematics (`animation.c`)

Instead of baking keyframe animations in Blender, *Abandon All Hope* animates both the survivor and every zombie **procedurally** in C:

1. **Gait Cycle (`foot_cycle`)**:
   Each foot has a phase `p` in `[0, 1)`. For the first `beta` of the cycle (e.g. `0.38` for a run, `0.60` for a zombie shamble), the boot is planted on the ground and slides backward under the body (`s: 0.5 -> -0.5`). For the remaining `1 - beta`, the foot lifts in a sine arc (`sin(PI * t)`) and pitches from toe-off to heel-strike.
2. **Two-Bone IK (`ik2`)**:
   Given a hip/shoulder joint `a`, a target `t` (foot or rifle grip), bone lengths `l1` and `l2`, and a bend direction `pole` (knees forward, elbows down-and-out), `ik2()` uses the **Law of Cosines** to solve the exact middle joint position (knee or elbow):
   ```c
   float x = (l1 * l1 - l2 * l2 + d * d) / (2.0f * d);
   float h = sqrtf(fmaxf(0.0f, l1 * l1 - x * x));
   ```
3. **Limb Orientation (`bone_transform`)**:
   `bone_transform(a, b, face)` builds a column-major `Matrix` whose `+Y` axis points from `b` to `a`, `+Z` faces `face`, and translation is the midpoint `(a + b) * 0.5`.

## 4.2 The Survivor Rig (`player.c`)

The survivor's rig splits into two coordinated halves:
- **Lower body (`mLower`)**: Turns toward the travel direction (or flips into a backpedal with hysteresis when running away from the aim line), capped at `HIP_MAX = 1.0 rad`.
- **Upper body (`mUpper`)**: Blades toward the gun aim (`-0.62 rad`), leans into the velocity vector, kicks back on shots, and tips the rifle in during reloads.
- **Hands**: The right hand stays locked to `GRIP_R` (pistol grip); the left hand stays on `GRIP_L` (handguard) and follows a 7-keyframe `reload_hand()` path from the magwell to the chest-rig pouch and back.


---

## Hands-On Examples

### 1. Asymmetric Gait Cycle: [`ex04_gait_cycle.c`](../examples/ex04_gait_cycle.c)
```bash
cmake -B build -DBUILD_LESSON_EXAMPLE=ex04_gait_cycle && cmake --build build --target ex04_gait_cycle
./build/ex04_gait_cycle
```
Visualizes `foot_cycle(p, beta)` with `beta = 0.62` (62% planted stance, 38% lifted swing):
```c
if (p < beta) {
    float u = p / beta;
    return (FootPose){ 0.5f - u, 0.0f, 0.0f }; // planted foot slides backward
}
float u = (p - beta) / (1.0f - beta);
return (FootPose){ -0.5f + 0.5f * (1.0f - cosf(PI * u)), sinf(PI * u), 0.35f * sinf(2.0f * PI * u) };
```
The two legs run `0.5` out of phase while `bob = -12.0f * cosf(2.0f * (phase * 2.0f * PI - PI * beta))` drives the double-frequency hip bounce.

### 2. Interactive 3D Two-Bone IK: [`ex04_two_bone_ik.c`](../examples/ex04_two_bone_ik.c)
```bash
cmake -B build -DBUILD_LESSON_EXAMPLE=ex04_two_bone_ik && cmake --build build --target ex04_two_bone_ik
./build/ex04_two_bone_ik
```
Move the red target with `WASD` + `Q/E` to watch `ik2(shoulder, target, l1, l2, pole)` solve the elbow joint using the Law of Cosines:
```c
float cosA = Clamp((l1 * l1 + d * d - l2 * l2) / (2.0f * l1 * d), -1.0f, 1.0f);
float sinA = sqrtf(fmaxf(0.0f, 1.0f - cosA * cosA));
Vector3 bend = Vector3Normalize(Vector3Subtract(pole, Vector3Scale(dir, Vector3DotProduct(pole, dir))));
return Vector3Add(a, Vector3Add(Vector3Scale(dir, l1 * cosA), Vector3Scale(bend, l1 * sinA)));
```
Projecting `pole` perpendicular to `dir` guarantees the knee or elbow always bends in a natural anatomical direction.
