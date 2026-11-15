#ifndef STORAGE_H
#define STORAGE_H

#include <stdbool.h>

typedef struct {
    int masterVol;  // 0..10
    int musicVol;   // 0..10
    bool shake;
    bool muted;
    int bestWave;
    int bestKills;
} GameSettings;

extern GameSettings gSettings;

void storage_load(void);
void storage_save(void);
bool storage_record_run(int wave, int kills);

#endif // STORAGE_H
