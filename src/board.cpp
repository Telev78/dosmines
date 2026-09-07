#include <stdlib.h>
#include <time.h>
#include <alloc.h>
#include "board.h"

Board::Board() : grid(NULL), W(0), H(0), mines(0),
                 firstClickDone(0), flagsCount(0), revealedCount(0) {}

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
    firstClickDone = 0;
    flagsCount = 0;
    revealedCount = 0;
}

void Board::placeMines(int safeX, int safeY) {
    srand((unsigned int)time(NULL));
    int p = 0;
    while (p < mines) {
        int i = rand() % (W * H);
        int x = i % W;
        int y = i / W;
        /* La case du premier clic ne doit JAMAIS contenir de mine */
        if (x == safeX && y == safeY) continue;
        if (!grid[i].isMine) {
            grid[i].isMine = 1;
            p++;
        }
    }

    for (int y = 0; y < H; y++) {
        for (int x = 0; x < W; x++) {
            if (grid[y * W + x].isMine) continue;
            int c = 0;
            for (int dy = -1; dy <= 1; dy++) {
                for (int dx = -1; dx <= 1; dx++) {
                    int nx = x + dx;
                    int ny = y + dy;
                    if (nx >= 0 && nx < W && ny >= 0 && ny < H && grid[ny * W + nx].isMine) {
                        c++;
                    }
                }
            }
            grid[y * W + x].count = c;
        }
    }
}

/* Retourne : 1 si succès, 0 si mine explosée (perdu), -1 si action ignorée (drapeau ou déjà révélée) */
int Board::reveal(int x, int y) {
    if (x < 0 || x >= W || y < 0 || y >= H) return -1;
    Cell &c = grid[y * W + x];
    if (c.isRevealed || c.isFlagged) return -1;

    if (!firstClickDone) {
        placeMines(x, y);
        firstClickDone = 1;
    }

    c.isRevealed = 1;
    revealedCount++;

    if (c.isMine) {
        c.isExploded = 1;
        return 0; /* Perdu */
    }

    /* Cascade (Flood-fill) si case vide */
    if (c.count == 0) {
        for (int dy = -1; dy <= 1; dy++) {
            for (int dx = -1; dx <= 1; dx++) {
                if (dx == 0 && dy == 0) continue;
                reveal(x + dx, y + dy);
            }
        }
    }
    return 1;
}

void Board::toggleFlag(int x, int y) {
    if (x < 0 || x >= W || y < 0 || y >= H) return;
    Cell &c = grid[y * W + x];
    if (c.isRevealed) return;

    if (c.isFlagged) {
        c.isFlagged = 0;
        flagsCount--;
    } else {
        c.isFlagged = 1;
        flagsCount++;
    }
}

void Board::chord(int x, int y, int &exploded) {
    exploded = 0;
    if (x < 0 || x >= W || y < 0 || y >= H) return;
    Cell &c = grid[y * W + x];
    if (!c.isRevealed || c.count == 0) return;

    /* Compter les drapeaux adjacents */
    int adjacentFlags = 0;
    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            int nx = x + dx;
            int ny = y + dy;
            if (nx >= 0 && nx < W && ny >= 0 && ny < H && grid[ny * W + nx].isFlagged) {
                adjacentFlags++;
            }
        }
    }

    if (adjacentFlags == c.count) {
        for (int dy2 = -1; dy2 <= 1; dy2++) {
            for (int dx2 = -1; dx2 <= 1; dx2++) {
                int nx = x + dx2;
                int ny = y + dy2;
                if (nx >= 0 && nx < W && ny >= 0 && ny < H) {
                    if (!grid[ny * W + nx].isRevealed && !grid[ny * W + nx].isFlagged) {
                        if (reveal(nx, ny) == 0) {
                            exploded = 1;
                        }
                    }
                }
            }
        }
    }
}

void Board::revealAllMines() {
    for (int y = 0; y < H; y++) {
        for (int x = 0; x < W; x++) {
            Cell &c = grid[y * W + x];
            if (c.isMine && !c.isFlagged && !c.isExploded) {
                c.isRevealed = 1;
            } else if (!c.isMine && c.isFlagged) {
                c.isFalseMine = 1;
            }
        }
    }
}

int Board::checkVictory() {
    return (revealedCount == (W * H - mines));
}

Cell& Board::get(int x, int y) {
    return grid[y * W + x];
}
