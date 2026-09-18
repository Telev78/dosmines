#include <graphics.h>
#include <dos.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
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

/* Classe interne de flux abstrait : fichier ou memoire far */
class BMPStream {
    FILE* f;
    const unsigned char far* mem;
    long memSize;
    long memPos;

public:
    BMPStream(FILE* file) : f(file), mem(NULL), memSize(0L), memPos(0L) {}
    BMPStream(const unsigned char far* m, long size) : f(NULL), mem(m), memSize(size), memPos(0L) {}

    int seek(long offset) {
        if (f) return fseek(f, offset, SEEK_SET);
        if (offset < 0 || offset > memSize) return -1;
        memPos = offset;
        return 0;
    }

    unsigned int read(void* dest, unsigned int bytes) {
        if (f) return (unsigned int)fread(dest, 1, bytes, f);
        if (memPos >= memSize) return 0;
        if (memPos + (long)bytes > memSize) bytes = (unsigned int)(memSize - memPos);
        _fmemcpy(dest, mem + memPos, bytes);
        memPos += bytes;
        return bytes;
    }
};

static int decodeBMPInternal(Video* vid, BMPStream &stream,
                            int &colHighlight, int &colShadow, int &colSurface, int &colSunkenBg,
                            void* digitSprites[12], void* emojiSprites[5],
                            void* cellSprites[8], void* numSprites[8]);

Video::Video() : spritesLoaded(0), graphInitialized(0),
               colHighlight(15), colShadow(8), colSunkenBg(0), colSurface(7), isVGA(0) {
    int i;
    for (i = 0; i < 12; i++) digitSprites[i] = NULL;
    for (i = 0; i < 5; i++)  emojiSprites[i] = NULL;
    for (i = 0; i < 8; i++)  cellSprites[i] = NULL;
    for (i = 0; i < 8; i++)  numSprites[i] = NULL;
}

int Video::init() {
    /* 1. D‚tection mat‚rielle du sous-systŠme graphique */
    int gdriver = DETECT, gmode;
    detectgraph(&gdriver, &gmode);

    /* D‚tection automatique de la carte graphique */
    if (gdriver == VGA) {
        gdriver = VGA;
        gmode = VGAHI;      /* Mode 640x480 16 couleurs */
        isVGA = 1;      
    } else if (gdriver == EGA) {
        gdriver = EGA;
        gmode = EGAHI;      /* Mode 640x350 16 couleurs */
        isVGA = 0;      
    } else {
        return 0; /* Carte non support‚e (CGA, Hercules, etc.) */
    }

    /* 2. Enregistrement du driver BGI et des polices vectorielles li‚es */
    registerbgidriver(EGAVGA_driver);
    registerbgifont(sansserif_font);
    registerbgifont(small_font);

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

/* Activation / Desactivation du signal video via le sequenceur VGA (port 0x3C4 / 0x3C5) */
void Video::setDisplayEnable(int enable) {
    /* Registre 1 du sequenceur VGA : Clocking Mode Register */
    /* Bit 5 (0x20) : Screen Off (1 = extinction / pas de balayage, 0 = affichage normal) */
    outportb(0x3C4, 0x01);
    unsigned char val = inportb(0x3C5);
    if (enable) {
        outportb(0x3C5, val & ~0x20); /* Rallume l'ecran */
    } else {
        outportb(0x3C5, val | 0x20);  /* Eteint l'ecran */
    }
}

int Video::loadSprites(const char* filepath) {
    FILE* f = fopen(filepath, "rb");
    if (!f) return 0;
    
    if (isVGA)
    {
        /* 1. Extinction mat‚rielle (s‚quenceur VGA pour vraie machine / 86Box) */
        setDisplayEnable(0);

        /* 2. Extinction de la palette DAC (tous les index … noir pour DOSBox) */
    
        int k;
        for (k = 0; k < 16; k++) {
            setVgaPaletteIndex(k, 0, 0, 0);
        }
    }

    BMPStream stream(f);
    int res = decodeBMPInternal(this, stream, colHighlight, colShadow, colSurface, colSunkenBg,
                                digitSprites, emojiSprites, cellSprites, numSprites);
    fclose(f);

    if (isVGA)
    {    
        /* 3. Rallumage mat‚riel du signal vid‚o une fois l'‚cran nettoy‚ */
        setDisplayEnable(1);
    }
    if (res) spritesLoaded = 1;
    return res;
}

int Video::loadSpritesFromMemory(const unsigned char far* data, long size) {
    if (!data || size <= 0L) return 0;

    if (isVGA)
    {
        /* 1. Extinction mat‚rielle (s‚quenceur VGA pour vraie machine / 86Box) */
        setDisplayEnable(0);

        /* 2. Extinction de la palette DAC (tous les index … noir pour DOSBox) */
        int k;
        for (k = 0; k < 16; k++) {
            setVgaPaletteIndex(k, 0, 0, 0);
        }
    }
    BMPStream stream(data, size);
    int res = decodeBMPInternal(this, stream, colHighlight, colShadow, colSurface, colSunkenBg,
                                digitSprites, emojiSprites, cellSprites, numSprites);

    if (isVGA)
    {                                
        /* 3. Rallumage mat‚riel du signal vid‚o une fois l'‚cran nettoy‚ */
        setDisplayEnable(1);
    }
    if (res) spritesLoaded = 1;
    return res;
}

static int decodeBMPInternal(Video* vid, BMPStream &stream,
                            int &colHighlight, int &colShadow, int &colSurface, int &colSunkenBg,
                            void* digitSprites[12], void* emojiSprites[5],
                            void* cellSprites[8], void* numSprites[8]) {
    (void)vid;
    BMPFileHeader bfh;
    BMPInfoHeader bih;

    if (stream.read(&bfh, sizeof(BMPFileHeader)) != sizeof(BMPFileHeader) || bfh.bfType != 0x4D42) {
        return 0;
    }

    if (stream.read(&bih, sizeof(BMPInfoHeader)) != sizeof(BMPInfoHeader) || bih.biBitCount != 4) {
        return 0;
    }

    /* 1. Lecture de la palette du BMP (stock‚e localement, PAS encore inject‚e au DAC pour garder l'‚cran noir) */
    int numColors = (int)bih.biClrUsed;
    if (numColors == 0 || numColors > 16) {
        numColors = 16; /* Format standard BMP 4-bits */
    }

    BMPColorEntry bmpPalette[16];
    stream.seek(sizeof(BMPFileHeader) + sizeof(BMPInfoHeader));
    int readColors = stream.read(bmpPalette, sizeof(BMPColorEntry) * numColors) / sizeof(BMPColorEntry);

    int imgW = (int)bih.biWidth;
    int imgH = (int)bih.biHeight;
    int rowStride = ((imgW * 4 + 31) / 32) * 4;

    /* Allocation d'un petit buffer pour une seule ligne (rowStride = 88 octets) */
    unsigned char* rowBuffer = (unsigned char*)malloc(rowStride);
    if (!rowBuffer) {
        return 0;
    }

    /* 2. Extraction directe des teintes de relief depuis le sprite de la cellule non-d‚voil‚e */
    int cellRowTop    = imgH - 1 - 50;
    int cellRowBottom = imgH - 1 - 65;
    int cellRowMid    = imgH - 1 - 58;

    stream.seek(bfh.bfOffBits + (long)cellRowTop * rowStride);
    stream.read(rowBuffer, rowStride);
    colHighlight = (rowBuffer[1 / 2] & 0x0F);

    stream.seek(bfh.bfOffBits + (long)cellRowBottom * rowStride);
    stream.read(rowBuffer, rowStride);
    colShadow = (rowBuffer[16 / 2] >> 4);

    stream.seek(bfh.bfOffBits + (long)cellRowMid * rowStride);
    stream.read(rowBuffer, rowStride);
    colSurface = (rowBuffer[8 / 2] >> 4);

    /* 3. Fond creus‚ des compteurs : couleur la plus sombre de la palette */
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

    /* Coordonn‚es de dessin temporaire sur l'‚cran d'initialisation (0,0) */
    int tempX = 0;
    int tempY = 0;
    int i, px, py;

    /* 4. Ligne 1 (Compteurs, Y=1) : 12 sprites de 13x23 pixels (s‚parateur 1px) */
    /* Ordre s‚quentiel : 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, tiret, vide */
    for (i = 0; i < 12; i++) {
        int srcX = 1 + i * 14;
        int srcY = 1;
        int sw = 13;
        int sh = 23;

        for (py = 0; py < sh; py++) {
            int bmpRow = imgH - 1 - (srcY + py);
            long fileOffset = bfh.bfOffBits + (long)bmpRow * rowStride;
            stream.seek(fileOffset);
            stream.read(rowBuffer, rowStride);

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

    /* 5. Ligne 2 (Emojis, Y=25) : 5 sprites de 24x24 pixels */
    /* Normal, Cliqu‚, Surpris, Victoire, D‚faite */
    for (i = 0; i < 5; i++) {
        int srcX = 1 + i * 25;
        int srcY = 25;
        int sw = 24;
        int sh = 24;

        for (py = 0; py < sh; py++) {
            int bmpRow = imgH - 1 - (srcY + py);
            long fileOffset = bfh.bfOffBits + (long)bmpRow * rowStride;
            stream.seek(fileOffset);
            stream.read(rowBuffer, rowStride);

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

    /* 6. Ligne 3 (Cellules, Y=50) : 8 sprites de 16x16 pixels */
    /* Non-r‚v‚l‚e, Vide, Drapeau, ?, ? enfonc‚, Mine, Mine rouge, Fausse mine */
    for (i = 0; i < 8; i++) {
        int srcX = 1 + i * 17;
        int srcY = 50;
        int sw = 16;
        int sh = 16;

        for (py = 0; py < sh; py++) {
            int bmpRow = imgH - 1 - (srcY + py);
            long fileOffset = bfh.bfOffBits + (long)bmpRow * rowStride;
            stream.seek(fileOffset);
            stream.read(rowBuffer, rowStride);

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

    /* 7. Ligne 4 (Chiffres de proximit‚ 1 … 8, Y=67) : 8 sprites de 16x16 pixels */
    for (i = 0; i < 8; i++) {
        int srcX = 1 + i * 17;
        int srcY = 67;
        int sw = 16;
        int sh = 16;

        for (py = 0; py < sh; py++) {
            int bmpRow = imgH - 1 - (srcY + py);
            long fileOffset = bfh.bfOffBits + (long)bmpRow * rowStride;
            stream.seek(fileOffset);
            stream.read(rowBuffer, rowStride);

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
    cleardevice();

    /* 8. Maintenant que l'‚cran est effac‚, on injecte les vraies couleurs de palette au DAC VGA */
    if (readColors > 0) {
        int palIdx;
        if (vid->getIsVGA() == 1)
        {
            /* ========================================================
               MODE VGA : Injection directe des couleurs 24-bits au DAC 
               ======================================================== */
            
            for (palIdx = 0; palIdx < readColors; palIdx++) {
                vid->setVgaPaletteIndex(palIdx, bmpPalette[palIdx].rgbRed, bmpPalette[palIdx].rgbGreen, bmpPalette[palIdx].rgbBlue);
            }
            /* Si le BMP a moins de 16 couleurs (ex: 14), restaurer le blanc standard pour l'index 15 */
            /* indispensable pour que le pilote souris DOS (qui utilise l'index 15 pour le blanc) ne devienne pas noir */
            for (palIdx = readColors; palIdx < 16; palIdx++) {
                vid->setVgaPaletteIndex(palIdx, 255, 255, 255);
            }
        }
        else {
            /* ========================================================
               MODE EGA : Conversion RGB 24-bits -> Palette EGA 6-bits (64 couleurs)
               ======================================================== */
            for (palIdx = 0; palIdx < readColors; palIdx++) {
                int r8 = bmpPalette[palIdx].rgbRed;
                int g8 = bmpPalette[palIdx].rgbGreen;
                int b8 = bmpPalette[palIdx].rgbBlue;
                
                /* Division brute par 85 : s‚pare parfaitement les nuances 0, 128, 192 et 255 */
                int vR = r8 / 85; 
                int vG = g8 / 85;
                int vB = b8 / 85;
                
                /* Extraction des signaux primaires et secondaires */
                int R = (vR >> 1) & 1;
                int r = vR & 1;
                int G = (vG >> 1) & 1;
                int g = vG & 1;
                int B = (vB >> 1) & 1;
                int b = vB & 1;
                
                /* Assemblage de l'octet EGA (rrggbb) */
                int egaColor = (r << 5) | (g << 4) | (b << 3) | (R << 2) | (G << 1) | B;
                
                /* Reprogrammation du registre d'attribut */
                setpalette(palIdx, egaColor);
            }
        }
    }
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

/* Affichage d'un compteur … 3 digits (format Windows : 000 … 999 ou n‚gatif) */
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
    setcolor(colSunkenBg);
}

void Video::setCreditColor() {
    /* Si la palette est bicolore (noir et blanc), colShadow est noir (0), ce qui se */
    /* confondrait avec le fond d'‚cran noir. On bascule alors sur colHighlight (blanc). */
    /* En mode couleur, colShadow (gris fonc‚) offre un contraste doux et feutr‚ sur fond noir. */
    if (colShadow == colSunkenBg) {
        setcolor(colHighlight);
    } else {
        setcolor(colShadow);
    }
}
