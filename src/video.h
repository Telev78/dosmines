#ifndef VIDEO_H
#define VIDEO_H

#include <stdio.h>

/* Constantes pour les indices de sprites */
/* Cellules (8 états) */
#define SPR_CELL_UNREVEALED 0
#define SPR_CELL_EMPTY      1
#define SPR_CELL_FLAG       2
#define SPR_CELL_QUESTION   3
#define SPR_CELL_Q_CLICKED  4
#define SPR_CELL_MINE       5
#define SPR_CELL_EXPLODED   6
#define SPR_CELL_FALSE_MINE 7

/* Emojis (5 états) */
#define EMOJI_NORMAL   0
#define EMOJI_CLICKED  1
#define EMOJI_SURPRISE 2
#define EMOJI_WON      3
#define EMOJI_LOST     4

/* Couleur de fond UI correspondant à l'index 11 du BMP (Gris clair 189,190,189) */
#define UI_BG_COLOR   11
#define UI_WHITE      13
#define UI_DARKGRAY   5
#define UI_BLACK      0

class Video {
    void* digitSprites[12]; /* 0=1..9=0, 10=tiret, 11=vide */
    void* emojiSprites[5];  /* 24x24 */
    void* cellSprites[8];   /* 16x16 */
    void* numSprites[8];    /* 1..8 (16x16) */
    int spritesLoaded;

    void setVgaPaletteIndex(int index, unsigned char r, unsigned char g, unsigned char b);

public:
    Video();
    ~Video();

    int  loadSprites(const char* filepath);
    void freeSprites();

    void drawCell(int x, int y, int type);
    void drawNumberCell(int x, int y, int num);
    void drawEmoji(int x, int y, int state);
    void drawDigit(int x, int y, int digitIdx);
    void drawCounter(int x, int y, int value);

    void drawBezel(int x, int y, int w, int h, int out);
    void drawSunkenRect(int x, int y, int w, int h);
};

#endif
