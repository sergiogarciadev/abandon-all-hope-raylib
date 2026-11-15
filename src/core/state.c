#include "state.h"
#include <string.h>

GameState gState = {0};

void state_reset_stats(void) {
    memset(&gState, 0, sizeof(gState));
    gState.maxHp = 3;
    gState.hp = 3;
    gState.magSize = 8;
    gState.mag = 8;
    gState.reloadTime = 1.2f;
    gState.fireRate = 0.13f;
    gState.damage = 1.0f;
    gState.speed = 6.2f;
    gState.zombieSpeed = 2.2f;
    gState.zombieHp = 2.0f;
}
