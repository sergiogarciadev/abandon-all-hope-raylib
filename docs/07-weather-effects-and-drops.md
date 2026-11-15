# Chapter 7: Weather, Effects, and Supply Drops

## 7.1 Dynamic Storm System (`weather.c`)

A single `level` (`0.0` to `1.0`) drives the entire atmosphere as waves climb:
- **Wave 4**: clear night (`0.0`)
- **Wave 5**: drizzle (`0.15`)
- **Wave 8**: steady rain (`0.40`)
- **Wave 12**: downpour with lightning & thunder (`0.70`)
- **Wave 15**: full howling storm (`1.00`)

In `weather_draw()`, we use Raylib's immediate-mode `rlBegin(RL_LINES)` to draw up to `2400` slanted rain streaks wrapping modulo `RAIN_TOP` around the player, plus expanding additive `DrawCircle3D` splash rings on the ground. Lightning flashes (`flashT`) briefly brighten the sky/fog color, moonlight, and ambient light in a double-stroke decay.

## 7.2 Blood, Casings, Smoke, and Debris (`particles.c`)

All transient 3D particles live in fixed-size pools (`MAX_BLOOD_PARTICLES = 256`, `MAX_DEBRIS = 256`)—zero `malloc`/`free` during gameplay:
- **Blood & Wood/Glass Chips**: Ballistic gravity (`-16 u/s²`), floor bounce at `y = 0.02`, and shrinking before expiry.
- **Brass Casings**: Ejected to the right of the rifle's ejection port with spin.
- **Gun Smoke**: Expands (`1 + 2.5*k`) and fades quadratically as it rises.

## 7.3 Supply Drops (`drops.c`)

Every zombie kill increments `sinceDrop`. With a `5%` base chance (plus `SCAVENGER` bonus) or after `30` dry kills (pity timer), a supply drop spawns:
- `+1 HEALTH` (suppressed when at full HP)
- `DOUBLE DAMAGE` (10s of 2× damage and hot orange tracers)
- `BOTTOMLESS MAG` (8s of infinite magazine)

Drops outside the cabin automatically slide inward so the player can always reach them, and pull toward the player within `2.5` units.


---

## Hands-On Example: [`ex07_storm_weather.c`](../examples/ex07_storm_weather.c)

```bash
cmake -B build -DBUILD_LESSON_EXAMPLE=ex07_storm_weather && cmake --build build --target ex07_storm_weather
./build/ex07_storm_weather
```

Press `[SPACE]` to trigger the double-exponential lightning flash from `src/world/weather.c`:

```c
if (flashT >= 0.0f) {
    flashT += dt;
    flash = expf(-flashT * 14.0f) + (flashT > 0.13f ? 0.8f * expf(-(flashT - 0.13f) * 9.0f) : 0.0f);
    if (flashT > 1.2f) flashT = -1.0f;
}
```
1. **Double Flash:** A sharp initial spike (`exp(-14t)`) followed `0.13s` later by a secondary stroke (`0.8 * exp(-9(t - 0.13))`) modulates the sky/fog color.
2. **Recycled Rain & Splashes:** `900` rain streaks wrap around `y < 0` and `80` ground splash rings (`DrawCircle3D`) expand and fade out over `0.35s`.
