#ifndef MINESAPP_H
#define MINESAPP_H

#include <time.h>
#include "common.h"
#include "mouse.h"
#include "video.h"
#include "board.h"
#include "scores.h"

class MinesApp
{
    Video v;
    Mouse m;
    Board b;
    ScoreManager scores;
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

    void renderDirtyCells();
    int menu();
    void play(Difficulty d);
    void showHighScores();
    void promptNewRecord(Difficulty d, int seconds);
    void computeLayout();
    void drawFullInterface(int isInitial = 0);
    void drawHeader();
    void drawGrid();
    void drawEmptyGrid();
    void drawSingleCell(int gx, int gy, int isDepressed = 0);

public:
    MinesApp();
    void run();
};

#endif
