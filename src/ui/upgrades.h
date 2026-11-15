#ifndef UPGRADES_H
#define UPGRADES_H

#include <stdbool.h>

void upgrades_open(bool healed);
void upgrades_close(void);
bool upgrades_update(float dt); // returns true when an upgrade was picked
void upgrades_draw(void);

#endif // UPGRADES_H
