#ifndef BOARD_H
#define BOARD_H

#include "common.h"

struct Cell {
    unsigned int isMine       : 1;
    unsigned int isRevealed   : 1;
    unsigned int isFlagged    : 1;
    unsigned int isQuestion   : 1;
    unsigned int isExploded   : 1;
    unsigned int isFalseMine  : 1;
    unsigned int count        : 4;
};

class Board {
    Cell* grid;
    int W, H, mines;
    int firstClickDone;
    int flagsCount;
    int revealedCount;

    void placeMines(int safeX, int safeY);

public:
    Board();
    ~Board();

    void  setup(Difficulty d);
    int   reveal(int x, int y);
    void  toggleFlag(int x, int y, int enableQuestionMarks = 1);
    void  chord(int x, int y, int &exploded);
    void  revealAllMines();

    int   checkVictory();

    Cell& get(int x, int y);
    int   getW() const { return W; }
    int   getH() const { return H; }
    int   getTotalMines() const { return mines; }
    int   getFlagsCount() const { return flagsCount; }
    int   getRemainingMines() const { return mines - flagsCount; }
    int   isStarted() const { return firstClickDone; }
};

#endif
