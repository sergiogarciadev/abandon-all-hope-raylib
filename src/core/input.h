#ifndef INPUT_H
#define INPUT_H

#include "raylib.h"
#include <stdbool.h>

void input_init(void);
void input_end_frame(void);

bool input_key_down(int key);
bool input_key_pressed(int key);
Vector2 input_mouse_pos(void);
bool input_mouse_down(int button);
bool input_mouse_pressed(int button);

#endif // INPUT_H
