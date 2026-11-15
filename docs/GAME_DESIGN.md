# Abandon All Hope — Game Design Document (GDD)

**Genre:** 3D Top-Down Zombie Survival Shooter  
**Engine & Language:** C11, Raylib 5.5, OpenGL 3.3 (GLSL 330), CMake + Ninja, LLVM (`clang` + `lld`)  
**Asset Footprint:** **Procedural 3D/Audio/Sprites** — every 3D mesh, texture, particle, UI icon, sound effect, and musical note is procedurally generated at runtime; only the 3 UI TrueType fonts (`Anton-Regular.ttf`, `Jersey25-Regular.ttf`, `JimNightshade-Regular.ttf`) are loaded from `assets/fonts/`.

---

## 1. High Concept & Core Loop

You are a lone survivor trapped inside a log cabin deep in a moonlit forest. Endless waves of zombies emerge from the dark woods, converge on the cabin, bash down wooden doors, and shatter and climb through barred windows.

### Core Gameplay Loop
1. **Title Screen (`SCREEN_TITLE`):** Live 3D cabin yard with 4 wandering zombies (`TITLE_ZOMBIES = 4`) under drifting moonlight (`TITLE_FOV = 42°`).
2. **Camera Swoop (`SWOOP_TIME = 1.8s`):** Clicking **PLAY** smoothly interpolates the camera position, target, and FOV (`42° → 55°`) into the top-down combat perspective (`CAM_OFFSET = (0, 30, 13)`).
3. **Combat Wave:**
   - Move (`WASD` / Arrows), aim with the mouse raycast onto `y = 0.9`, fire (`Left Mouse`, hold for auto), and reload (`R`).
   - Zombies pathfind around solids (Dijkstra routing), attack doors/windows, or climb through broken windows.
   - Slain zombies topple into persistent corpses with expanding blood pools and have a chance to drop supply pickups (`+1 HEALTH`, `2X DAMAGE`, `INF AMMO`).
4. **Wave Clear & Intermission (`1.6s`):**
   - Survivor automatically heals **+1 HP** (up to `maxHp`).
   - Calm music plays and a wave-cleared banner appears.
5. **Upgrade Draft:** Choose 1 of 3 randomly dealt upgrade cards (mouse click or `1` / `2` / `3`) from 10 distinct upgrades.
6. **Escalation & Death:** Waves grow faster, tougher, and stormier until `hp <= 0`, triggering `SCREEN_DEAD` and saving the best run (`bestWave`, `bestKills`) to `aah_settings.cfg`.

---

## 2. Player Stats, Controls & Combat

### Starting Stats (`src/core/state.c`)
| Stat | Initial Value | Notes |
|---|---:|---|
| `hp` / `maxHp` | `3 / 3` | Heart-shaped pips in HUD; `+1` healed after each wave |
| `mag` / `magSize` | `8 / 8` | Rounds per magazine |
| `reloadTime` | `1.2s` | Time to swap magazines |
| `fireRate` | `0.13s` | ~7.7 shots/sec |
| `damage` | `1.0` | Multiplied by `2.0` while `2X DAMAGE` drop is active |
| `pierce` | `0` | Extra zombies a bullet can pass through |
| `spread` | `0` | Extra bullets per shot (`1 + spread`, `0.13 rad` fan) |
| `speed` | `6.2 u/s` | Movement speed |
| `dropBonus` | `0.0` | Added to base `DROP_CHANCE` (`5%`) |
| `HURT_IFRAMES` | `1.5s` | Post-hit invulnerability with visibility flicker + red screen flash |

### Controls
| Input | Action |
|---|---|
| `W A S D` / Arrow Keys | Move survivor (diagonal normalized, solid collision push-out) |
| `Mouse Move` | Aim rifle (screen-to-world ray intersected with plane `y = 0.9`) |
| `Left Mouse` (hold) | Fire weapon |
| `R` | Reload magazine |
| `1` / `2` / `3` or Click | Pick upgrade card |
| `M` | Toggle audio mute |
| `ESC` / `P` | Pause / Resume (`SetExitKey(KEY_NULL)` prevents accidental exit) |

---

## 3. Level & Wave Progression

### Wave Scaling (`src/game.c`)
Each wave `n` computes:
- **Zombie Speed:** $\min(5.2,\; 2.3 + 0.18n)$ units/sec
- **Zombie HP:** $2.0 + 0.4(n - 1)$
- **Spawn Count:** $\max(0,\; 5 + 3n - \text{alive})$
- **Spawn Interval:** $\max(0.35,\; 1.25 - 0.06n)$ seconds (capped at `MAX_ALIVE = 30` simultaneous zombies)

### Wave Omens
| Wave | Omen Banner |
|---|---|
| Wave 1 | *"they are already here"* |
| Wave 5 | *"it starts to rain"* |
| Wave 8 | *"the rain sets in"* |
| Wave 12 | *"the storm breaks"* |
| Wave 15 | *"god help you"* |
| All other waves | *"it gets worse"* |

### Upgrades (`src/ui/upgrades.c`)
Ten upgrades across four color-coded categories (`UPG_FIREPOWER`, `UPG_HANDLING`, `UPG_SURVIVAL`, `UPG_FORTUNE`), each rendered with a custom 8×8 1-bit/2-tone (`X` solid, `o` dim) pixel icon:

| ID | Category | Name | Effect | Max Rank |
|---|---|---|---|---:|
| `mag` | Handling | **EXTENDED MAG** | `+4` rounds per magazine & refills mag | 5 |
| `reload` | Handling | **QUICK HANDS** | `reloadTime *= 0.75` (25% faster) | 4 |
| `damage` | Firepower | **HOLLOW POINTS** | `+0.5` bullet damage | 6 |
| `rate` | Firepower | **HAIR TRIGGER** | `fireRate *= 0.80` (20% faster) | 4 |
| `pierce` | Firepower | **PIERCING** | `+1` zombie pierce | 3 |
| `spread` | Firepower | **BUCKSHOT** | `+1` bullet per shot | 3 |
| `hp` | Survival | **THICK SKIN** | `+1` max HP and heals `+1` HP | 3 |
| `speed` | Survival | **FLEET FOOT** | `speed *= 1.12` (12% faster) | 3 |
| `luck` | Fortune | **SCAVENGER** | `+3%` supply drop chance | 3 |
| `heal` | Survival | **PATCH UP** | Restores `hp = maxHp` (only offered when injured) | Unlimited |

### Supply Drops (`src/entities/drops.c`)
- **Spawn Chance:** `DROP_CHANCE (5%) + gState.dropBonus`, with a **pity timer** (`DROP_PITY = 30` kills without a drop guarantees one).
- **Drop Types:**
  - `DROP_MED` (`+1 HEALTH`): Restores 1 HP (or `+10s` `2X DAMAGE` if already at full HP).
  - `DROP_DMG` (`2X DAMAGE`): Doubles bullet damage and turns tracers/muzzle flash orange-red for `10s`.
  - `DROP_AMMO` (`INF AMMO`): Infinite magazine ammo for `10s`.
- **Behavior:** Drops bob vertically, slide out of cabin walls if spawned on an edge, magnetically pull toward the player within `2.6` units, and expire after `DROP_LIFE = 12s`.

---

## 4. Environment, Ambient & Weather

### World Layout (`src/world/world.c`, `src/world/openings.c`)
- **Ground:** `200×200` plane with a procedural mossy earth & grass texture.
- **Cabin:** Log-walled structure with plank flooring, central rug, interior debris, **2 breakable wooden doors**, and **3 barred/glazed windows** (`openings.c`).
- **Yard & Perimeter:** Woodpile, chopping block with axe, abandoned pickup truck, flickering yard lantern (dynamic point light #1), fence posts, tree stumps, and a fallen trunk.
- **Forest:** `900` procedural pine/dead trees and `1000` ground brush clusters surrounding the clearing.
- **Navigation (`src/world/solids.c`):** Axis-aligned 2D bounding boxes (`push_out_of_solids`, `seg_hits_box`, `clear_path`) plus a precomputed waypoint graph using **Dijkstra's shortest-path algorithm** (`route_around`) so zombies navigate cleanly around exterior obstacles and cabin walls.

### Storm Weather System (`src/world/weather.c`)
Storm intensity `L` (`0.0 → 1.0`) ramps smoothly across waves (`STORM_STEPS`: Wave 4 `0.0`, Wave 5 `0.15`, Wave 8 `0.40`, Wave 12 `0.70`, Wave 15+ `1.0`):
- **Rain:** `2400` falling rain streaks (`RAIN_COUNT`) scrolling in a `72×72` box around the camera.
- **Ground Splashes:** `240` expanding ring ripples (`SPLASH_COUNT`) spawned across the yard proportional to `L`.
- **Lightning & Thunder:** When `L >= BOLT_LEVEL (0.7)`, periodic lightning triggers a **double-exponential flash** (`exp(-14t) + 0.8*exp(-9(t-0.13))`) that briefly boosts moonlight, ambient light, and shifts fog color from `#05060a` toward `#3a4458`, followed by distance-delayed thunder (`0.15s–2.35s`).
- **Fog Compression:** `fogNear` tightens from `32 → 18` and `fogFar` from `62 → 44` as the storm intensifies.

---

## 5. Procedural Meshes & Shaders

### Chamfered Block Geometry (`src/core/blocks.c`)
- Instead of plain boxes or external `.obj`/`.glb` files, models are built from `BlockSpec` arrays using `bevelBox()` — a custom 45°-chamfered cuboid generator that produces crisp highlights along every edge.
- `BlockBatch` merges multiple blocks into a single static `Mesh` with baked **per-vertex colors** (optional per-block noise jitter via `SeededRng`), minimizing draw calls.

### Custom GLSL 330 Lighting & Fog Shader (`src/core/scene.c`)
`LIT_VS` / `LIT_FS` renders all 3D world and character geometry with:
- **Directional Moonlight** + **Hemisphere Ambient**
- **2 Dynamic Point Lights:**
  1. Yard lantern (warm flickering amber)
  2. Player muzzle flash (bright yellow-white, or orange-red during `2X DAMAGE`)
- **Emissive Uniform (`emissive`):** Used for glowing eyes, lantern glass, and hit flashes.
- **Optional Texturing (`useTex`):** Toggles between pure vertex-colored blocks and textured surfaces (ground plane, survivor's procedural `8×8` plaid flannel shirt).
- **Distance Fog:** Smoothly blends fragments into `gScene.fogColor` between `fogNear` and `fogFar`.

---

## 6. Animations & Visual Effects

### Survivor Rig (`src/entities/player.c`, `src/entities/animation.c`)
- **Lower-Body / Upper-Body Decoupling:** `hipYaw` lags and aligns toward movement direction (automatically flipping `back` when backpedaling), while `mUpper` twists toward the mouse `aim` angle.
- **Procedural Gait (`foot_cycle`):** Asymmetric stance/swing phase (`BETA = 0.62`) drives stride length, foot lift, ankle pitch, pelvis bob, and lateral sway.
- **Two-Bone IK (`ik2` + `bone_transform`):** Solves knee and elbow positions analytically in 3D for both legs and both arms (`GRIP_R`, `GRIP_L`).
- **Gun Firing Animation:**
  - `gPlayer.kick` pitches the rifle muzzle up and kicks the stock backward (`mGun`), decaying exponentially (`exp(-14*dt)`).
  - Spawns an additive **muzzle flash** + dynamic point light (`muzzleLightInt`), **tracer bullets** (`38 u/s`), **spinning brass cartridge casings** (`DEBRIS_CASING`) ejected to the right with gravity/floor bounce, and rising **gun smoke** (`DEBRIS_SMOKE`).
- **Gun Reload Animation:**
  - `gPlayer.reload` smoothly tilts the head down (`+0.32 rad`) and cants the rifle (`0.65 rad` roll).
  - At normalized reload progress `u = 0.22`, the empty magazine detaches and drops to the ground as physical debris (`DEBRIS_MAG`).
  - The left hand follows a **7-keyframe cubic-eased path** (`reload_hand_pos`) from the forend, down to the empty mag, to the waist pouch, and back up to seat a fresh magazine at `u = 0.95`.

### Zombie Rig & Animations (`src/entities/zombies.c`)
- **4 Baked Kits:** Varied heights, skin tones, tattered clothing, and hair/headwear built at startup.
- **Procedural Gait:** Randomized per-zombie `limp`, `limpSide`, `hunch`, head `tilt`, `arms` reach (`0`, `1`, or `2` outstretched arms), and `groan` jaw-drop.
- **Door & Window Attacks:** Zombies rear back and slam into closed doors (`o->jolt` spring oscillation + wood chip bursts `DEBRIS_WOOD`) or crack/shatter windows (`DEBRIS_GLASS` shards).
- **Window Climbing (`climb_step`):** Once a window is broken (`o->climberIdx`), a zombie claims the opening and executes a **3-phase procedural climb** over the sill (`Z_CLIMB_UP`, `Z_CLIMB_OVER`, `Z_CLIMB_DROP`), pitching its torso forward and pulling its knees up over the frame before dropping inside.
- **Death & Corpses:** Slain zombies spawn directional **blood spray particles**, topple backward onto the ground (`Corpse`), and leak an expanding **blood pool** decal (`poolScale`).

---

## 7. Procedural Audio & Adaptive Music

### Software Synthesizer (`src/audio/synth.c`)
- Runs on Raylib's `AudioStream` callback at **44,100 Hz, 16-bit stereo**, synchronized with a **`PTHREAD_MUTEX_RECURSIVE`** mutex (preventing self-deadlock when the music sequencer spawns voices from inside the audio callback).
- **Oscillators:** Band-limited additive wavetables (`WAVE_SINE`, `WAVE_TRI`, `WAVE_PULSE12`, `WAVE_PULSE25`, `WAVE_PULSE50`, `WAVE_SAW`), **15-bit LFSR noise** (`WAVE_NOISE_LONG`, `WAVE_NOISE_METAL`, `WAVE_NOISE_WHITE`), and a **dual-formant vocal synthesizer** (`WAVE_GROWL`).
- **Per-Voice Processing:** Pitch sweeps (`f → f2`), delayed vibrato, detune, ADSR/exponential envelopes, and a **State-Variable Filter (SVF)** (`FILT_LOWPASS`, `FILT_BANDPASS`, `FILT_HIGHPASS`).
- **Master Bus:** Ambient storm wind generator, dotted-8th stereo echo (`0.375s`), 4-comb Schroeder reverb, sidechain ducking, pause/death low-pass muffle, and `tanhf()` soft limiter.

### Spatialized SFX (`src/audio/sfx.c`)
All sound effects compute stereo pan and distance attenuation relative to `gPlayer.position`:
- Gunshots (layered noise crack + low punch + bolt click; boosted when `2X DAMAGE` or `spread > 0`)
- Zombie flesh hits, death groans, claw swipes
- Door bashes, door breaks, glass cracks, glass shatters
- Player footsteps (triggered on `foot_cycle` phase wrap), mag-out / mag-in reload clicks, hurt grunt, death sting, pickup chimes, thunder rumbles, and UI card deal/hover/upgrade/click sounds.

### Adaptive Sequencer (`src/audio/music.c`)
A real-time D-minor / F-major chiptune step sequencer (`THEME`, `SOAR`, `OMEN`, `HAVEN` across an 18-chord progression) that transitions seamlessly between:
- `MUSIC_TITLE` — Sparse atmospheric arpeggio on the main menu
- `MUSIC_CALM` — Quiet breather between waves
- `MUSIC_COMBAT` — Driving bass, kick/snare, and melody scaling with `wave`, `alive` count, and `lowHp`
- `MUSIC_DEAD` — Slow descending minor cadence on death

---

## 8. UI, Menus & Persistence

- **Typography & Distressed Shader (`src/ui/fonts.c`):** Loads `Anton-Regular.ttf` (144px), `JimNightshade-Regular.ttf` (80px), and `Jersey25-Regular.ttf` (36px) from `assets/fonts/`. `font_draw_distressed()` applies a GLSL screen-space alpha mask (`DISTRESS_FS`) sampled from a `600×300` 4-octave fractal noise texture (`clamp(-9 * n + 6.58, 0, 1)`) for chipped-paint transparency on `ABANDON` (`57px`) / `ALL HOPE` (`144px`), `PAUSED` (`117px`), and `YOU DIED` (`132px`).
- **HUD (`src/ui/hud.c`):** Top-left `WAVE / KILLS / LEFT` panel, top-center **8×7 pixel-art heart HP pips**, active `2X DAMAGE` / `INF AMMO` timers, bottom-center magazine bullet pips & reload progress bar, `Jim Nightshade` wave omen banners, supply toasts, and crimson edge-vignette damage flash.
- **Upgrade Picker (`src/ui/upgrades.c`):** 2-tone (`X` solid / `o` dim) 8×8 pixel-art icons, `Jim Nightshade` `WAVE X CLEARED` header, `Anton` card titles, and `Jersey 25` descriptions.
- **Menus (`src/ui/title.c`):** `SCREEN_TITLE` (`PLAY`, `HOW TO PLAY`, `SETTINGS`, `QUIT`), `SCREEN_PAUSED` (`RESUME`, `SETTINGS`, `QUIT TO TITLE`), and `SCREEN_DEAD` (`TRY AGAIN`, `QUIT TO TITLE`).
- **Persistence (`src/core/storage.c`):** Saves `masterVol`, `musicVol`, `shake`, `muted`, `bestWave`, and `bestKills` to `aah_settings.cfg`.
