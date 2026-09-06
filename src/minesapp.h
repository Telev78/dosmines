#ifndef MINESAPP_H
#define MINESAPP_H

#include "common.h"
#include "mouse.h"
#include "video.h"
#include "board.h"

class MinesApp {
    Video v;
    Mouse m;
    Board b;
    int curX, curY, curB, lastB;
    int offX, offY;

    void menu();
    void play(Difficulty d);
    void drawBoard();

public:
    MinesApp();
    void run();
};

#endif
