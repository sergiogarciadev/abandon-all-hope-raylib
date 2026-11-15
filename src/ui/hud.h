#ifndef HUD_H
#define HUD_H

void hud_init(void);
void hud_reset(void);
void hud_toast(const char *msg);
void hud_wave_banner(const char *title, const char *sub);
void hud_damage_flash(void);
void hud_update(float dt);
void hud_draw(void);

#endif // HUD_H
