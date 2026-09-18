#include <stdio.h>
#include <string.h>
#include "scores.h"

#define SCORE_FILE "SCORES.DAT"

ScoreManager::ScoreManager() {
    reset();
    load();
}

void ScoreManager::reset() {
    strcpy(scores[0].name, STR_MENU_SCORES_DEFAULT);
    scores[0].time = 999;

    strcpy(scores[1].name, STR_MENU_SCORES_DEFAULT);
    scores[1].time = 999;

    strcpy(scores[2].name, STR_MENU_SCORES_DEFAULT);
    scores[2].time = 999;
}

void ScoreManager::load() {
    FILE* f = fopen(SCORE_FILE, "rb");
    if (!f) return;

    fread(scores, sizeof(HighScore), 3, f);
    fclose(f);

    /* Verifications de securite sur les valeurs chargees */
    for (int i = 0; i < 3; i++) {
        scores[i].name[15] = '\0';
        if (scores[i].time < 1) scores[i].time = 1;
        if (scores[i].time > 999) scores[i].time = 999;
    }
}

void ScoreManager::save() {
    FILE* f = fopen(SCORE_FILE, "wb");
    if (!f) return;

    fwrite(scores, sizeof(HighScore), 3, f);
    fclose(f);
}

int ScoreManager::isNewRecord(Difficulty d, int seconds) const {
    int idx = (int)d;
    if (idx < 0 || idx > 2) return 0;
    return (seconds < scores[idx].time);
}

void ScoreManager::update(Difficulty d, const char* name, int seconds) {
    int idx = (int)d;
    if (idx < 0 || idx > 2) return;

    strncpy(scores[idx].name, name, 15);
    scores[idx].name[15] = '\0';
    if (scores[idx].name[0] == '\0') {
        strcpy(scores[idx].name, STR_MENU_SCORES_DEFAULT);
    }
    scores[idx].time = seconds;
    save();
}

const HighScore& ScoreManager::get(Difficulty d) const {
    int idx = (int)d;
    if (idx < 0 || idx > 2) idx = 0;
    return scores[idx];
}
