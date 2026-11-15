# Chapter 9: Creating the UI & Full Game Loop

In the browser version, the HUD, upgrade cards, and title/pause/death overlays were built with HTML and CSS. In Raylib, we render everything in immediate mode directly inside `BeginDrawing()` / `EndDrawing()` after `EndMode3D()`, keeping the entire game self-contained with zero external UI libraries.

---

## 1. Persistent Settings & High Scores (`src/core/storage.c`)

To replace `localStorage`, `storage_load()` and `storage_save()` read and write a plain `key=value` file (`aah_settings.cfg`) in the working directory:

- `masterVol` (`0..10`), `musicVol` (`0..10`), `shake` (`0`/`1`), `muted` (`0`/`1`)
- `bestWave` and `bestKills`

`storage_record_run(wave, kills)` compares the finished run against the stored best (ranked by `kills`, then `wave`) and persists a new record immediately.

---

## 2. The In-Game HUD (`src/ui/hud.c`)

`hud_draw()` renders clean, readable 2D overlays scaled to `GetScreenWidth()` and `GetScreenHeight()`:

- **Damage Vignette Flash**: `hud_damage_flash()` starts a `0.55`s timer that draws four edge gradients (`DrawRectangleGradientV` / `DrawRectangleGradientH`) in crimson when the survivor is hit.
- **Wave / Kills / Remaining**: Top-left panel showing `gState.wave`, `gState.kills`, and `gState.toSpawn + gZombieCount`.
- **Health Pips & Powerups**: Top-center row of **8×7 pixel-art heart pips** (`HEART_PIX`, crimson when remaining, dark gray when lost), plus live countdowns for `2X DAMAGE` (`gState.pDamage`) and `INF AMMO` (`gState.pAmmo`).
- **Magazine & Reload Bar**: Bottom-center bullet pips (`gState.mag` / `gState.magSize`), `[R] RELOAD` prompt when low, and an animated reload progress bar (`gState.reloadT / gState.reloadTime`).
- **Wave Banners & Toasts**: `hud_wave_banner(title, sub)` announces each wave and its omen (`"they are already here"`, `"it starts to rain"`, etc.) in `Jim Nightshade` horror script, while `hud_toast(msg)` briefly displays supply pickups and mute toggles.

---

## 3. Custom Styled Fonts & Distressed Shader (`src/ui/fonts.c`)

To match the browser version's typography, three TrueType fonts in `assets/fonts/` (copied post-build by `CMakeLists.txt`) are loaded in `fonts_init()`:

- **`Anton-Regular.ttf` (144px atlas):** Heavy condensed display font for `ABANDON` (`57px`) / `ALL HOPE` (`144px`), `PAUSED` (`117px`), `YOU DIED` (`132px`), menu buttons, and upgrade card names.
- **`JimNightshade-Regular.ttf` (80px atlas):** Gothic horror script for wave banners and `WAVE X CLEARED`.
- **`Jersey25-Regular.ttf` (36px atlas):** Retro pixel-styled UI font for subtitles, descriptions, stats, and hints.

For `.distressed` titles, `build_grit_mask()` generates a `600×300` 4-octave fractal noise texture (`baseFrequency = 0.03`, `seed = 4`) thresholded with `clamp(-9 * n + 6.58, 0, 1)`, and `DISTRESS_FS` multiplies fragment alpha in screen space to carve true transparent chipped-paint holes through both the letters and their drop shadow.

---

## 4. Upgrade Card Picker & 8×8 Pixel Icons (`src/ui/upgrades.c`)

Between waves, the survivor automatically heals **+1 HP** (up to `maxHp`), and `upgrades_open(bool healed)` filters the 10 upgrades (`UPGRADE_DEFS`) by `maxLevel` (and `hp < maxHp` for `PATCH UP`), shuffles the eligible pool with Fisher-Yates, and deals up to 3 cards:

- **2-Tone 8×8 Pixel Art Icons**: Each upgrade stores an 8-row string pattern (`'X'` = solid category color, `'o'` = dim translucent, `'.'` = transparent). `draw_pixel_icon()` renders crisp `7×7` squares with `DrawRectangle()` — no sprite sheets needed.
- **Deal Animation & Input**: Cards slide up smoothly with `ease_smooth()`, lift on mouse hover (`CheckCollisionPointRec` + `sfx_hover`), and can be chosen with either a mouse click or keys `1`, `2`, `3`.

---

## 5. Title Screen, Camera Swoop & Menus (`src/ui/title.c`)

Rather than a static background, the title screen (`SCREEN_TITLE`) renders the live 3D cabin yard with 4 wandering zombies (`TITLE_ZOMBIES = 4`) while the camera gently drifts at `TITLE_FOV = 42.0f`:

- Clicking **PLAY** calls `title_start_swoop()`, which interpolates `cam->position`, `cam->target`, and `cam->fovy` over `SWOOP_TIME = 1.8f` seconds using `ease_smooth()` straight into the top-down gameplay camera (`GAME_FOV = 55.0f`).
- **QUIT** on `SCREEN_TITLE` sets `title_should_quit()` so the desktop loop exits cleanly.
- `SetExitKey(KEY_NULL)` in `src/main.c` stops Raylib from closing the window on `ESC`, allowing `ESC` or `P` to toggle `SCREEN_PAUSED` cleanly.
- `SCREEN_HOWTO`, `SCREEN_SETTINGS`, and `SCREEN_DEAD` reuse a lightweight immediate-mode `ui_button()` helper with hover highlights and `sfx_ui("click")`.

---

## 6. Tying Everything Together (`src/game.c` & `src/main.c`)

`src/game.c` coordinates the complete state machine:

1. `game_init()` initializes storage, scene, world, weather, effects, player, zombies, drops, fonts, HUD, synth, and music, then enters `SCREEN_TITLE`.
2. `game_update(dt)` handles global `M` mute and `ESC`/`P` pause, updates the upgrade picker or active wave spawning (`MAX_ALIVE = 30`, scaling `zombieSpeed` and `zombieHp` per wave), steps physics/AI/bullets/particles/weather/audio, and applies screen shake (`gSettings.shake`).
3. `game_draw()` renders the 3D scene followed by `hud_draw()`, `upgrades_draw()`, and `title_draw()`.
4. `game_cleanup()` unloads fonts/shaders, shuts down the audio stream, and frees all GPU meshes cleanly.

---

**Congratulations!** You now have a complete, procedural 3D zombie survival shooter in C11 and Raylib.
