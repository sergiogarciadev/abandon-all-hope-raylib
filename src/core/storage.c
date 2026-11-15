#include "storage.h"
#include "core/math_util.h"
#include <stdio.h>
#include <string.h>

#define SETTINGS_FILE "aah_settings.cfg"

GameSettings gSettings = {
    .masterVol = 7,
    .musicVol = 5,
    .shake = true,
    .muted = false,
    .bestWave = 0,
    .bestKills = 0,
};

void storage_load(void) {
    FILE *f = fopen(SETTINGS_FILE, "r");
    if (!f) return;

    char line[128];
    while (fgets(line, sizeof(line), f)) {
        char key[64];
        int val = 0;
        if (sscanf(line, "%63[^=]=%d", key, &val) == 2) {
            if (strcmp(key, "masterVol") == 0) gSettings.masterVol = (int)clampf((float)val, 0.0f, 10.0f);
            else if (strcmp(key, "musicVol") == 0) gSettings.musicVol = (int)clampf((float)val, 0.0f, 10.0f);
            else if (strcmp(key, "shake") == 0) gSettings.shake = (val != 0);
            else if (strcmp(key, "muted") == 0) gSettings.muted = (val != 0);
            else if (strcmp(key, "bestWave") == 0) gSettings.bestWave = val > 0 ? val : 0;
            else if (strcmp(key, "bestKills") == 0) gSettings.bestKills = val > 0 ? val : 0;
        }
    }
    fclose(f);
}

void storage_save(void) {
    FILE *f = fopen(SETTINGS_FILE, "w");
    if (!f) return;
    fprintf(f, "masterVol=%d\n", gSettings.masterVol);
    fprintf(f, "musicVol=%d\n", gSettings.musicVol);
    fprintf(f, "shake=%d\n", gSettings.shake ? 1 : 0);
    fprintf(f, "muted=%d\n", gSettings.muted ? 1 : 0);
    fprintf(f, "bestWave=%d\n", gSettings.bestWave);
    fprintf(f, "bestKills=%d\n", gSettings.bestKills);
    fclose(f);
}

bool storage_record_run(int wave, int kills) {
    if (kills <= 0 && wave <= 1 && gSettings.bestKills > 0) return false;
    bool isBest = (kills > gSettings.bestKills) ||
                  (kills == gSettings.bestKills && wave > gSettings.bestWave);
    if (isBest) {
        gSettings.bestWave = wave;
        gSettings.bestKills = kills;
        storage_save();
    }
    return isBest;
}
