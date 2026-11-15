#ifndef STATE_H
#define STATE_H

#include <stdbool.h>

#define UPGRADE_COUNT 10

typedef struct {
    bool running;
    bool paused;
    int hp;
    int maxHp;
    int kills;
    int shots;
    int wave;
    int mag;
    int magSize;
    bool reloading;
    float reloadT;
    float reloadTime;
    float fireCd;
    float fireRate;
    float dmgCd;

    float damage;
    int pierce;
    int spread;
    float speed;
    float dropBonus;
    int upgrades[UPGRADE_COUNT];

    float pDamage;
    float pAmmo;
    int sinceDrop;

    int toSpawn;
    float spawnTimer;
    float intermission;
    bool choosing;
    float zombieSpeed;
    float zombieHp;
    float shake;
} GameState;

extern GameState gState;

void state_reset_stats(void);

#endif // STATE_H
