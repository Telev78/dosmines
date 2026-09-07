#include <graphics.h>
#include <stdlib.h>
#include <stdio.h>
#include <alloc.h>
#include "video.h"

/* Structure du Header BMP (format Windows 3.1) */
#pragma pack(1)
struct BMPFileHeader {
    unsigned int  bfType;
    unsigned long bfSize;
    unsigned int  bfReserved1;
    unsigned int  bfReserved2;
    unsigned long bfOffBits;
};

struct BMPInfoHeader {
    unsigned long  biSize;
    long           biWidth;
    long           biHeight;
    unsigned short biPlanes;
    unsigned short biBitCount;
    unsigned long  biCompression;
    unsigned long  biSizeImage;
    long           biXPelsPerMeter;
    long           biYPelsPerMeter;
    unsigned long  biClrUsed;
    unsigned long  biClrImportant;
};
#pragma pack()

Video::Video() : spritesLoaded(0) {
    int i;
    for (i = 0; i < 12; i++) digitSprites[i] = NULL;
    for (i = 0; i < 5; i++)  emojiSprites[i] = NULL;
    for (i = 0; i < 8; i++)  cellSprites[i] = NULL;
    for (i = 0; i < 8; i++)  numSprites[i] = NULL;

    registerbgidriver(EGAVGA_driver);
    int gdir = VGA, gmod = VGAHI;
    initgraph(&gdir, &gmod, "");
}

Video::~Video() {
    freeSprites();
    closegraph();
}

void Video::freeSprites() {
    int i;
    for (i = 0; i < 12; i++) {
        if (digitSprites[i]) { free(digitSprites[i]); digitSprites[i] = NULL; }
    }
    for (i = 0; i < 5; i++) {
        if (emojiSprites[i]) { free(emojiSprites[i]); emojiSprites[i] = NULL; }
    }
    for (i = 0; i < 8; i++) {
        if (cellSprites[i]) { free(cellSprites[i]); cellSprites[i] = NULL; }
    }
    for (i = 0; i < 8; i++) {
        if (numSprites[i]) { free(numSprites[i]); numSprites[i] = NULL; }
    }
    spritesLoaded = 0;
}

/* Correspondance exacte palette BMP 4-bits -> Palette standard Borland BGI (16 couleurs) */
int Video::mapBmpColorToVGA(unsigned char bmpIndex) {
    switch (bmpIndex) {
        case 0:  return BLACK;
        case 1:  return BLUE;
        case 2:  return LIGHTBLUE;
        case 3:  return RED;
        case 4:  return LIGHTRED;
        case 5:  return DARKGRAY;
        case 6:  return GREEN;
        case 7:  return CYAN;
        case 8:  return BROWN;
        case 9:  return MAGENTA;
        case 10: return -1; /* Vert fluo : transparent ! */
        case 11: return LIGHTGRAY;
        case 12: return YELLOW;
        case 13: return WHITE;
        case 14:
        case 15:
        default: return LIGHTGRAY;
    }
}

int Video::loadSprites(const char* filepath) {
    FILE* f = fopen(filepath, "rb");
    if (!f) return 0;

    BMPFileHeader bfh;
    BMPInfoHeader bih;

    if (fread(&bfh, sizeof(BMPFileHeader), 1, f) != 1 || bfh.bfType != 0x4D42) {
        fclose(f);
        return 0;
    }

    if (fread(&bih, sizeof(BMPInfoHeader), 1, f) != 1 || bih.biBitCount != 4) {
        fclose(f);
        return 0;
    }

    int imgW = (int)bih.biWidth;
    int imgH = (int)bih.biHeight;
    int rowStride = ((imgW * 4 + 31) / 32) * 4;

    /* Allocation d'un petit buffer pour une seule ligne (rowStride = 88 octets) */
    unsigned char* rowBuffer = (unsigned char*)malloc(rowStride);
    if (!rowBuffer) {
        fclose(f);
        return 0;
    }

    /* Coordonnées de dessin temporaire sur l'écran d'initialisation (0,0) */
    int tempX = 0;
    int tempY = 0;
    int i, px, py;

    /* 1. Ligne 1 (Compteurs, Y=1) : 12 sprites de 13x23 pixels (séparateur 1px) */
    /* Ordre séquentiel : 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, tiret, vide */
    for (i = 0; i < 12; i++) {
        int srcX = 1 + i * 14;
        int srcY = 1;
        int sw = 13;
        int sh = 23;

        for (py = 0; py < sh; py++) {
            int bmpRow = imgH - 1 - (srcY + py);
            long fileOffset = bfh.bfOffBits + (long)bmpRow * rowStride;
            fseek(f, fileOffset, SEEK_SET);
            fread(rowBuffer, 1, rowStride, f);

            for (px = 0; px < sw; px++) {
                int curX = srcX + px;
                unsigned char byteVal = rowBuffer[curX / 2];
                unsigned char nibble = (curX % 2 == 0) ? (byteVal >> 4) : (byteVal & 0x0F);
                int color = mapBmpColorToVGA(nibble);
                putpixel(tempX + px, tempY + py, (color == -1) ? BLACK : color);
            }
        }

        unsigned int sz = imagesize(tempX, tempY, tempX + sw - 1, tempY + sh - 1);
        digitSprites[i] = malloc(sz);
        if (digitSprites[i]) {
            getimage(tempX, tempY, tempX + sw - 1, tempY + sh - 1, digitSprites[i]);
        }
    }

    /* 2. Ligne 2 (Emojis, Y=25) : 5 sprites de 24x24 pixels */
    /* Normal, Cliqué, Surpris, Victoire, Défaite */
    for (i = 0; i < 5; i++) {
        int srcX = 1 + i * 25;
        int srcY = 25;
        int sw = 24;
        int sh = 24;

        for (py = 0; py < sh; py++) {
            int bmpRow = imgH - 1 - (srcY + py);
            long fileOffset = bfh.bfOffBits + (long)bmpRow * rowStride;
            fseek(f, fileOffset, SEEK_SET);
            fread(rowBuffer, 1, rowStride, f);

            for (px = 0; px < sw; px++) {
                int curX = srcX + px;
                unsigned char byteVal = rowBuffer[curX / 2];
                unsigned char nibble = (curX % 2 == 0) ? (byteVal >> 4) : (byteVal & 0x0F);
                int color = mapBmpColorToVGA(nibble);
                putpixel(tempX + px, tempY + py, (color == -1) ? LIGHTGRAY : color);
            }
        }

        unsigned int sz = imagesize(tempX, tempY, tempX + sw - 1, tempY + sh - 1);
        emojiSprites[i] = malloc(sz);
        if (emojiSprites[i]) {
            getimage(tempX, tempY, tempX + sw - 1, tempY + sh - 1, emojiSprites[i]);
        }
    }

    /* 3. Ligne 3 (Cellules, Y=50) : 8 sprites de 16x16 pixels */
    /* Non-révélée, Vide, Drapeau, ?, ? enfoncé, Mine, Mine rouge, Fausse mine */
    for (i = 0; i < 8; i++) {
        int srcX = 1 + i * 17;
        int srcY = 50;
        int sw = 16;
        int sh = 16;

        for (py = 0; py < sh; py++) {
            int bmpRow = imgH - 1 - (srcY + py);
            long fileOffset = bfh.bfOffBits + (long)bmpRow * rowStride;
            fseek(f, fileOffset, SEEK_SET);
            fread(rowBuffer, 1, rowStride, f);

            for (px = 0; px < sw; px++) {
                int curX = srcX + px;
                unsigned char byteVal = rowBuffer[curX / 2];
                unsigned char nibble = (curX % 2 == 0) ? (byteVal >> 4) : (byteVal & 0x0F);
                int color = mapBmpColorToVGA(nibble);
                putpixel(tempX + px, tempY + py, (color == -1) ? LIGHTGRAY : color);
            }
        }

        unsigned int sz = imagesize(tempX, tempY, tempX + sw - 1, tempY + sh - 1);
        cellSprites[i] = malloc(sz);
        if (cellSprites[i]) {
            getimage(tempX, tempY, tempX + sw - 1, tempY + sh - 1, cellSprites[i]);
        }
    }

    /* 4. Ligne 4 (Chiffres proximité 1 à 8, Y=67) : 8 sprites de 16x16 pixels */
    for (i = 0; i < 8; i++) {
        int srcX = 1 + i * 17;
        int srcY = 67;
        int sw = 16;
        int sh = 16;

        for (py = 0; py < sh; py++) {
            int bmpRow = imgH - 1 - (srcY + py);
            long fileOffset = bfh.bfOffBits + (long)bmpRow * rowStride;
            fseek(f, fileOffset, SEEK_SET);
            fread(rowBuffer, 1, rowStride, f);

            for (px = 0; px < sw; px++) {
                int curX = srcX + px;
                unsigned char byteVal = rowBuffer[curX / 2];
                unsigned char nibble = (curX % 2 == 0) ? (byteVal >> 4) : (byteVal & 0x0F);
                int color = mapBmpColorToVGA(nibble);
                putpixel(tempX + px, tempY + py, (color == -1) ? LIGHTGRAY : color);
            }
        }

        unsigned int sz = imagesize(tempX, tempY, tempX + sw - 1, tempY + sh - 1);
        numSprites[i] = malloc(sz);
        if (numSprites[i]) {
            getimage(tempX, tempY, tempX + sw - 1, tempY + sh - 1, numSprites[i]);
        }
    }

    free(rowBuffer);
    fclose(f);
    spritesLoaded = 1;
    cleardevice();
    return 1;
}

void Video::drawCell(int x, int y, int type) {
    if (spritesLoaded && type >= 0 && type < 8 && cellSprites[type]) {
        putimage(x, y, cellSprites[type], COPY_PUT);
    } else {
        drawBezel(x, y, 15, 15, (type == SPR_CELL_UNREVEALED) ? 1 : 0);
    }
}

void Video::drawNumberCell(int x, int y, int num) {
    if (num >= 1 && num <= 8) {
        if (spritesLoaded && numSprites[num - 1]) {
            putimage(x, y, numSprites[num - 1], COPY_PUT);
            return;
        }
    }
    drawCell(x, y, SPR_CELL_EMPTY);
}

void Video::drawEmoji(int x, int y, int state) {
    if (spritesLoaded && state >= 0 && state < 5 && emojiSprites[state]) {
        putimage(x, y, emojiSprites[state], COPY_PUT);
    } else {
        drawBezel(x - 1, y - 1, 26, 26, (state == EMOJI_CLICKED) ? 0 : 1);
    }
}

void Video::drawDigit(int x, int y, int digitIdx) {
    if (spritesLoaded && digitIdx >= 0 && digitIdx < 12 && digitSprites[digitIdx]) {
        putimage(x, y, digitSprites[digitIdx], COPY_PUT);
    }
}

/* Affichage d'un compteur à 3 digits (format Windows : 000 à 999 ou négatif) */
void Video::drawCounter(int x, int y, int value) {
    if (value < -99) value = -99;
    if (value > 999) value = 999;

    int d[3];
    if (value < 0) {
        d[0] = 10; /* Tiret '-' (index 10) */
        int pos = -value;
        d[1] = (pos / 10) % 10;
        d[2] = pos % 10;
        /* Mappage : chiffre 1..9 -> index 0..8, chiffre 0 -> index 9 */
        d[1] = (d[1] == 0) ? 9 : (d[1] - 1);
        d[2] = (d[2] == 0) ? 9 : (d[2] - 1);
    } else {
        int v0 = (value / 100) % 10;
        int v1 = (value / 10) % 10;
        int v2 = value % 10;

        d[0] = (v0 == 0) ? 9 : (v0 - 1);
        d[1] = (v1 == 0) ? 9 : (v1 - 1);
        d[2] = (v2 == 0) ? 9 : (v2 - 1);
    }

    drawSunkenRect(x - 1, y - 1, 13 * 3 + 2, 23 + 2);
    drawDigit(x, y, d[0]);
    drawDigit(x + 13, y, d[1]);
    drawDigit(x + 26, y, d[2]);
}

void Video::drawBezel(int x, int y, int w, int h, int out) {
    setcolor(out ? WHITE : DARKGRAY);
    line(x, y, x + w, y);
    line(x, y, x, y + h);
    setcolor(out ? DARKGRAY : WHITE);
    line(x + w, y, x + w, y + h);
    line(x, y + h, x + w, y + h);
}

void Video::drawSunkenRect(int x, int y, int w, int h) {
    setcolor(DARKGRAY);
    line(x, y, x + w, y);
    line(x, y, x, y + h);
    setcolor(WHITE);
    line(x + w, y, x + w, y + h);
    line(x, y + h, x + w, y + h);
    setfillstyle(SOLID_FILL, BLACK);
    bar(x + 1, y + 1, x + w - 1, y + h - 1);
}
