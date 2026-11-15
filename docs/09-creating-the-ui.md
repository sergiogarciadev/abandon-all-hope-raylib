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
- **Health Pips & Powerups**: Top-center row of health bars (`gState.hp` / `gState.maxHp`), plus live countdowns for `2X DAMAGE` (`gState.pDamage`) and `INF AMMO` (`gState.pAmmo`).
- **Magazine & Reload Bar**: Bottom-center bullet pips (`gState.mag` / `gState.magSize`), `[R] RELOAD` prompt when low, and an animated reload progress bar (`gState.reloadT / gState.reloadTime`).
- **Wave Banners & Toasts**: `hud_wave_banner(title, sub)` announces each wave and its omen (`"they are already here"`, `"it starts to rain"`, etc.), while `hud_toast(msg)` briefly displays supply pickups and mute toggles.

---

## 3. Upgrade Card Picker & 8×8 Pixel Icons (`src/ui/upgrades.c`)

Between waves, `upgrades_open()` filters the 10 upgrades (`UPGRADE_DEFS`) by `maxLevel` (and `hp < maxHp` for `PATCH UP`), shuffles the eligible pool with Fisher-Yates, and deals up to 3 cards:

- **1-Bit Pixel Art Icons**: Each upgrade stores an 8-byte bitmap (`uint8_t icon[8]`). `draw_pixel_icon()` tests each bit `(0x80 >> c)` and draws crisp `7×7` squares with `DrawRectangle()` — no sprite sheets needed.
- **Deal Animation & Input**: Cards slide up smoothly with `ease_smooth()`, lift on mouse hover (`CheckCollisionPointRec` + `sfx_hover`), and can be chosen with either a mouse click or keys `1`, `2`, `3`.

---

## 4. Title Screen, Camera Swoop & Menus (`src/ui/title.c`)

Rather than a static background, the title screen (`SCREEN_TITLE`) renders the live 3D cabin yard with 4 wandering zombies (`TITLE_ZOMBIES = 4`) while the camera gently drifts at `TITLE_FOV = 42.0f`:

- Clicking **PLAY** calls `title_start_swoop()`, which interpolates `cam->position`, `cam->target`, and `cam->fovy` over `SWOOP_TIME = 1.8f` seconds using `ease_smooth()` straight into the top-down gameplay camera (`GAME_FOV = 55.0f`).
- `SetExitKey(KEY_NULL)` in `src/main.c` stops Raylib from closing the window on `ESC`, allowing `ESC` or `P` to toggle `SCREEN_PAUSED` cleanly.
- `SCREEN_HOWTO`, `SCREEN_SETTINGS`, and `SCREEN_DEAD` reuse a lightweight immediate-mode `ui_button()` helper with hover highlights and `sfx_ui("click")`.

---

## 5. Tying Everything Together (`src/game.c` & `src/main.c`)

`src/game.c` coordinates the complete state machine:

1. `game_init()` initializes storage, scene, world, weather, effects, player, zombies, drops, HUD, synth, and music, then enters `SCREEN_TITLE`.
2. `game_update(dt)` handles global `M` mute and `ESC`/`P` pause, updates the upgrade picker or active wave spawning (`MAX_ALIVE = 30`, scaling `zombieSpeed` and `zombieHp` per wave), steps physics/AI/bullets/particles/weather/audio, and applies screen shake (`gSettings.shake`).
3. `game_draw()` renders the 3D scene followed by `hud_draw()`, `upgrades_draw()`, and `title_draw()`.
4. `game_cleanup()` shuts down the audio stream and frees all GPU meshes and shaders cleanly.

---

**Congratulations!** You now have a complete, zero-asset, procedural 3D zombie survival shooter in C11 and Raylib.
