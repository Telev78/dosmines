#ifndef BOARD_H
#define BOARD_H

#include "common.h"

struct Cell {
    int isMine;
    int isRevealed;
    int isFlagged;
    int count;
};

class Board {
    Cell* grid;
    int W, H, mines;

    void placeMines();

public:
    Board();
    ~Board();

    void  setup(Difficulty d);
    void  reveal(int x, int y);
    Cell& get(int x, int y);
    int   getW();
    int   getH();
};

#endif
