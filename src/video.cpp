#include <graphics.h>
#include <dos.h>
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

struct BMPColorEntry {
    unsigned char rgbBlue;
    unsigned char rgbGreen;
    unsigned char rgbRed;
    unsigned char rgbReserved;
};
#pragma pack()

Video::Video() : spritesLoaded(0), graphInitialized(0),
               colHighlight(15), colShadow(8), colSunkenBg(0), colSurface(7) {
    int i;
    for (i = 0; i < 12; i++) digitSprites[i] = NULL;
    for (i = 0; i < 5; i++)  emojiSprites[i] = NULL;
    for (i = 0; i < 8; i++)  cellSprites[i] = NULL;
    for (i = 0; i < 8; i++)  numSprites[i] = NULL;
}

int Video::init() {
    /* 1. Détection matérielle du sous-système graphique */
    int gdriver = DETECT, gmode;
    detectgraph(&gdriver, &gmode);

    /* Le jeu exige impérativement une carte VGA (640x480 16 couleurs, Mode 12h) */
    if (gdriver != VGA) {
        return 0;
    }

    /* 2. Enregistrement du driver BGI lié et initialisation */
    registerbgidriver(EGAVGA_driver);
    gdriver = VGA;
    gmode   = VGAHI;
    initgraph(&gdriver, &gmode, "");

    int err = graphresult();
    if (err != grOk) {
        return 0;
    }

    graphInitialized = 1;
    return 1;
}

Video::~Video() {
    freeSprites();
    if (graphInitialized) {
        closegraph();
    }
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

/* Envoi direct au DAC VGA (ports 0x3C8 / 0x3C9, composantes sur 6 bits 0..63) */
void Video::setVgaPaletteIndex(int index, unsigned char r, unsigned char g, unsigned char b) {
    /* Table de correspondance Registres d'Attributs EGA standard 16 couleurs -> Registres DAC VGA */
    static const unsigned char egaToDac[16] = {
        0, 1, 2, 3, 4, 5, 20, 7, 56, 57, 58, 59, 60, 61, 62, 63
    };

    unsigned char dacEntry = (index >= 0 && index < 16) ? egaToDac[index] : (unsigned char)index;

    outportb(0x3C8, dacEntry);
    outportb(0x3C9, r >> 2);
    outportb(0x3C9, g >> 2);
    outportb(0x3C9, b >> 2);
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

        /* 1. Lecture de la palette du BMP et injection matérielle dans le DAC VGA */
    int numColors = (int)bih.biClrUsed;
    if (numColors == 0 || numColors > 16) {
        numColors = 16; /* Format standard BMP 4-bits */
    }

            BMPColorEntry bmpPalette[16];
    fseek(f, sizeof(BMPFileHeader) + sizeof(BMPInfoHeader), SEEK_SET);
    int readColors = fread(bmpPalette, sizeof(BMPColorEntry), numColors, f);
    if (readColors > 0) {
        int palIdx;
        for (palIdx = 0; palIdx < readColors; palIdx++) {
            setVgaPaletteIndex(palIdx, bmpPalette[palIdx].rgbRed, bmpPalette[palIdx].rgbGreen, bmpPalette[palIdx].rgbBlue);
        }
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

    /* 1. Extraction directe depuis le sprite de la cellule non-dévoilée */
    int cellRowTop    = imgH - 1 - 50;
    int cellRowBottom = imgH - 1 - 65;
    int cellRowMid    = imgH - 1 - 58;

    fseek(f, bfh.bfOffBits + (long)cellRowTop * rowStride, SEEK_SET);
    fread(rowBuffer, 1, rowStride, f);
    colHighlight = (rowBuffer[1 / 2] & 0x0F);

    fseek(f, bfh.bfOffBits + (long)cellRowBottom * rowStride, SEEK_SET);
    fread(rowBuffer, 1, rowStride, f);
    colShadow = (rowBuffer[16 / 2] >> 4);

    fseek(f, bfh.bfOffBits + (long)cellRowMid * rowStride, SEEK_SET);
    fread(rowBuffer, 1, rowStride, f);
    colSurface = (rowBuffer[8 / 2] >> 4);

    /* 2. Fond creusé des compteurs : couleur la plus sombre de la palette */
    long minLum = 30000L;
    colSunkenBg = 0;
    if (readColors > 0) {
        int lumIdx;
        for (lumIdx = 0; lumIdx < readColors; lumIdx++) {
            long lum = (long)bmpPalette[lumIdx].rgbRed * 30L +
                       (long)bmpPalette[lumIdx].rgbGreen * 59L +
                       (long)bmpPalette[lumIdx].rgbBlue * 11L;
            if (lum < minLum) {
                minLum = lum;
                colSunkenBg = lumIdx;
            }
        }
    }

    /* Coordonnées de dessin temporaire sur l'écran d'initialisation (0,0) */
    int tempX = 0;
    int tempY = 0;
    int i, px, py;

        /* 2. Ligne 1 (Compteurs, Y=1) : 12 sprites de 13x23 pixels (séparateur 1px) */
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
                putpixel(tempX + px, tempY + py, (int)nibble);
            }
        }

        unsigned int sz = imagesize(tempX, tempY, tempX + sw - 1, tempY + sh - 1);
        digitSprites[i] = malloc(sz);
        if (digitSprites[i]) {
            getimage(tempX, tempY, tempX + sw - 1, tempY + sh - 1, digitSprites[i]);
        }
    }

    /* 3. Ligne 2 (Emojis, Y=25) : 5 sprites de 24x24 pixels */
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
                putpixel(tempX + px, tempY + py, (int)nibble);
            }
        }

        unsigned int sz = imagesize(tempX, tempY, tempX + sw - 1, tempY + sh - 1);
        emojiSprites[i] = malloc(sz);
        if (emojiSprites[i]) {
            getimage(tempX, tempY, tempX + sw - 1, tempY + sh - 1, emojiSprites[i]);
        }
    }

    /* 4. Ligne 3 (Cellules, Y=50) : 8 sprites de 16x16 pixels */
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
                putpixel(tempX + px, tempY + py, (int)nibble);
            }
        }

        unsigned int sz = imagesize(tempX, tempY, tempX + sw - 1, tempY + sh - 1);
        cellSprites[i] = malloc(sz);
        if (cellSprites[i]) {
            getimage(tempX, tempY, tempX + sw - 1, tempY + sh - 1, cellSprites[i]);
        }
    }

    /* 5. Ligne 4 (Chiffres de proximité 1 à 8, Y=67) : 8 sprites de 16x16 pixels */
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
                putpixel(tempX + px, tempY + py, (int)nibble);
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
    setcolor(out ? colHighlight : colShadow);
    line(x, y, x + w, y);
    line(x, y, x, y + h);
    setcolor(out ? colShadow : colHighlight);
    line(x + w, y, x + w, y + h);
    line(x, y + h, x + w, y + h);
}

void Video::drawSunkenRect(int x, int y, int w, int h) {
    setcolor(colShadow);
    line(x, y, x + w, y);
    line(x, y, x, y + h);
    setcolor(colHighlight);
    line(x + w, y, x + w, y + h);
    line(x, y + h, x + w, y + h);
    setfillstyle(SOLID_FILL, colSunkenBg);
    bar(x + 1, y + 1, x + w - 1, y + h - 1);
}

void Video::drawPanel(int x, int y, int w, int h, int out) {
    /* Fond plein de la surface */
    setfillstyle(SOLID_FILL, colSurface);
    bar(x + 1, y + 1, x + w - 1, y + h - 1);
    /* Biseau 3D */
    drawBezel(x, y, w, h, out);
}

void Video::setTextColor() {
    /* Choix automatique de la couleur de texte pour un contraste maximal sur colSurface */
    /* Si colHighlight et colShadow sont identiques, on utilise colSunkenBg */
    setcolor(colSunkenBg);
}
