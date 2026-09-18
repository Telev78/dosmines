#ifndef VIDEO_H
#define VIDEO_H

#include <stdio.h>

/* Constantes pour les indices de sprites */
/* Cellules (8 ‚tats) */
#define SPR_CELL_UNREVEALED 0
#define SPR_CELL_EMPTY      1
#define SPR_CELL_FLAG       2
#define SPR_CELL_QUESTION   3
#define SPR_CELL_Q_CLICKED  4
#define SPR_CELL_MINE       5
#define SPR_CELL_EXPLODED   6
#define SPR_CELL_FALSE_MINE 7

/* Emojis (5 ‚tats) */
#define EMOJI_NORMAL   0
#define EMOJI_CLICKED  1
#define EMOJI_SURPRISE 2
#define EMOJI_WON      3
#define EMOJI_LOST     4

class Video {
    void* digitSprites[12]; /* 0=1..9=0, 10=tiret, 11=vide */
    void* emojiSprites[5];  /* 24x24 */
    void* cellSprites[8];   /* 16x16 */
    void* numSprites[8];    /* 1..8 (16x16) */
    int spritesLoaded;
    int graphInitialized;
    unsigned int isVGA : 1;

    /* R“les de palette UI d‚termin‚s dynamiquement d'aprŠs les couleurs du BMP */
    int colHighlight; /* LumiŠre du biseau 3D (couleur la plus claire) */
    int colShadow;    /* Ombre du biseau 3D (couleur sombre interm‚diaire) */
    int colSunkenBg;  /* Fond creus‚ des compteurs LED (couleur la plus sombre) */
    int colSurface;   /* Fond moyen de l'interface */

public:
    void setVgaPaletteIndex(int index, unsigned char r, unsigned char g, unsigned char b);
    void setDisplayEnable(int enable);

    Video();
    ~Video();

    int  init();
    int  loadSprites(const char* filepath);
    int  loadSpritesFromMemory(const unsigned char far* data, long size);
    void freeSprites();

    void drawCell(int x, int y, int type);
    void drawNumberCell(int x, int y, int num);
    void drawEmoji(int x, int y, int state);
    void drawDigit(int x, int y, int digitIdx);
    void drawCounter(int x, int y, int value);

    void drawBezel(int x, int y, int w, int h, int out);
    void drawSunkenRect(int x, int y, int w, int h);
    void drawPanel(int x, int y, int w, int h, int out);
    void setTextColor();
    void setCreditColor();
    unsigned int getIsVGA() const { return isVGA; }
};

#endif
