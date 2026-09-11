#ifndef MINESAPP_H
#define MINESAPP_H

#include <time.h>
#include "common.h"
#include "mouse.h"
#include "video.h"
#include "board.h"

class MinesApp {
    Video v;
    Mouse m;
    Board b;
    GameState state;

    int gridX, gridY;
    int boardWidthPx, boardHeightPx;
    int headerX, headerY, headerW, headerH;
    int emojiX, emojiY;
    int mineCounterX, timerCounterX, counterY;

    time_t gameStartTime;
    int elapsedSeconds;
    int emojiState;
    int questionMarksEnabled;

    /* Suivi de la case enfoncée sous la souris pour annulation au relâchement */
    int pressedCellX, pressedCellY;

    void menu();
    void play(Difficulty d);
    void computeLayout();
    void drawFullInterface();
    void drawHeader();
    void drawGrid();
    void drawSingleCell(int gx, int gy, int isDepressed = 0);

public:
    MinesApp();
    void run();
};

#endif
