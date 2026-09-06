#include <stdlib.h>
#include <time.h>
#include <alloc.h>
#include "board.h"

Board::Board() : grid(NULL), W(0), H(0), mines(0) {}

Board::~Board() {
    if (grid) {
        free(grid);
        grid = NULL;
    }
}

void Board::setup(Difficulty d) {
    if (grid) {
        free(grid);
        grid = NULL;
    }
    if (d == BEGINNER) {
        W = 9;  H = 9;  mines = 10;
    } else if (d == INTERMEDIATE) {
        W = 16; H = 16; mines = 40;
    } else {
        W = 30; H = 16; mines = 99;
    }
    grid = (Cell*)calloc(W * H, sizeof(Cell));
    placeMines();
}

void Board::placeMines() {
    srand((unsigned int)time(NULL));
    int p = 0;
    while (p < mines) {
        int i = rand() % (W * H);
        if (!grid[i].isMine) {
            grid[i].isMine = 1;
            p++;
        }
    }
    for (int y = 0; y < H; y++) {
        for (int x = 0; x < W; x++) {
            if (grid[y * W + x].isMine) continue;
            int c = 0;
            for (int i = -1; i <= 1; i++) {
                for (int j = -1; j <= 1; j++) {
                    int nx = x + j;
                    int ny = y + i;
                    if (nx >= 0 && nx < W && ny >= 0 && ny < H && grid[ny * W + nx].isMine) {
                        c++;
                    }
                }
            }
            grid[y * W + x].count = c;
        }
    }
}

void Board::reveal(int x, int y) {
    if (x < 0 || x >= W || y < 0 || y >= H) return;
    Cell &c = grid[y * W + x];
    if (c.isRevealed || c.isFlagged) return;
    c.isRevealed = 1;
    if (c.count == 0 && !c.isMine) {
        for (int i = -1; i <= 1; i++) {
            for (int j = -1; j <= 1; j++) {
                reveal(x + j, y + i);
            }
        }
    }
}

Cell& Board::get(int x, int y) {
    return grid[y * W + x];
}

int Board::getW() {
    return W;
}

int Board::getH() {
    return H;
}
