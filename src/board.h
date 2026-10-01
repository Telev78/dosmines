#ifndef BOARD_H
#define BOARD_H

#include "common.h"

// Structure pour stocker les coordonnées des cases modifiées
struct Point
{
    int x;
    int y;
};

struct Cell
{
    unsigned int isMine : 1;
    unsigned int isRevealed : 1;
    unsigned int isFlagged : 1;
    unsigned int isQuestion : 1;
    unsigned int isExploded : 1;
    unsigned int isFalseMine : 1;
    unsigned int count : 4;
};

class Board
{
    Cell *grid;
    int W, H, mines;
    int firstClickDone;
    int flagsCount;
    int revealedCount;

    // Tampon pour la Dirty List (taille max de la grille : 30x16 = 480)
    Point dirtyCells[480];
    int dirtyCount;

    void addDirty(int x, int y);
    void placeMines(int safeX, int safeY);

public:
    Board();
    ~Board();

    void clearDirty() { dirtyCount = 0; }
    int getDirtyCount() const { return dirtyCount; }
    const Point *getDirtyCells() const { return dirtyCells; }

    void setup(Difficulty d);
    int reveal(int x, int y);
    void toggleFlag(int x, int y, int enableQuestionMarks = 1);
    void chord(int x, int y, int &exploded);
    void revealAllMines();

    int checkVictory();

    Cell &get(int x, int y);
    int getW() const { return W; }
    int getH() const { return H; }
    int getTotalMines() const { return mines; }
    int getFlagsCount() const { return flagsCount; }
    int getRemainingMines() const { return mines - flagsCount; }
    int isStarted() const { return firstClickDone; }
};

#endif
