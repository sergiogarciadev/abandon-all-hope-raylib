#ifndef FONTS_H
#define FONTS_H

#include "raylib.h"

extern Font gFontAnton;
extern Font gFontJersey;
extern Font gFontNightshade;

void fonts_init(void);
void fonts_cleanup(void);

float font_measure(Font font, const char *text, float size, float spacing);
void font_draw(Font font, const char *text, float x, float y, float size, float spacing, Color color);
void font_draw_shadow(Font font, const char *text, float x, float y, float size, float spacing,
                      Color color, Color shadow, float off);
void font_draw_distressed(Font font, const char *text, float x, float y, float size, float spacing,
                          Color color, unsigned int seed);

#endif // FONTS_H
