#ifndef SCORES_H
#define SCORES_H

#include "common.h"

struct HighScore {
    char name[16];
    int time;
};

class ScoreManager {
    HighScore scores[3];

public:
    ScoreManager();
    void load();
    void save();
    void reset();
    int  isNewRecord(Difficulty d, int seconds) const;
    void update(Difficulty d, const char* name, int seconds);
    const HighScore& get(Difficulty d) const;
};

#endif
