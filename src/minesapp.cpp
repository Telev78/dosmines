#include <graphics.h>
#include <dos.h>
#include <stdio.h>
#include <conio.h>
#include "minesapp.h"
#include "assetdef.h"

MinesApp::MinesApp() : state(STATE_MENU), emojiState(EMOJI_NORMAL) {
}

void MinesApp::run() {
    /* 1. Vérification matérielle VGA */
    if (!v.init()) {
        printf("\n=======================================================\n");
        printf(" ERREUR : Carte graphique VGA non detectee !\n");
        printf(" Ce jeu necessite une carte VGA et le mode 640x480x16.\n");
        printf(" Les cartes EGA, CGA et Hercules ne sont pas supportees.\n");
        printf("=======================================================\n\n");
        return;
    }

    /* 2. Initialisation de la souris */
    if (!m.init()) {
        closegraph();
        printf("\nErreur : Pilote souris DOS non detecte (int 33h) !\n");
        return;
    }

    /* 3. Chargement de la sprite sheet :
          Essai de chargement externe prioritaire (asset.bmp / asset/asset.bmp),
          puis fallback transparent sur la sprite-sheet embarquée dans le binaire */
    int loaded = v.loadSprites("asset.bmp") ||
                 v.loadSprites("asset\\asset.bmp") ||
                 v.loadSprites("asset/asset.bmp") ||
                 v.loadSprites("minesweepersprite.bmp") ||
                 v.loadSprites("asset\\minesweepersprite.bmp");

    if (!loaded) {
        v.loadSpritesFromMemory(default_asset_bmp, sizeof(default_asset_bmp));
    }

    menu();
}

void MinesApp::computeLayout() {
    boardWidthPx  = b.getW() * 16;
    boardHeightPx = b.getH() * 16;

    /* Centrage de la grille dans l'écran 640x480 */
    gridX = (640 - boardWidthPx) / 2;
    /* Pour la grille avancée (256px de haut), on laisse de la place pour le header */
    gridY = (480 - boardHeightPx) / 2 + 25;

    headerW = boardWidthPx;
    headerH = 37;
    headerX = gridX;
    headerY = gridY - headerH - 6;

    emojiX = headerX + (headerW - 24) / 2;
    emojiY = headerY + (headerH - 24) / 2;

    mineCounterX  = headerX + 6;
    timerCounterX = headerX + headerW - (13 * 3) - 6;
    counterY      = headerY + (headerH - 23) / 2;
}

void MinesApp::drawFullInterface() {
    cleardevice();

    /* Cadre principal englobant (look Windows 3.1) avec fond opaque */
    v.drawPanel(headerX - 6, headerY - 6, headerW + 12, (gridY + boardHeightPx + 6) - (headerY - 6), 1);

    /* En-tête */
    v.drawPanel(headerX, headerY, headerW, headerH, 0);

    /* Grille en relief creusé */
    v.drawBezel(gridX - 3, gridY - 3, boardWidthPx + 6, boardHeightPx + 6, 0);

    drawHeader();
    drawGrid();
}

void MinesApp::drawHeader() {
    /* Compteur de mines restantes */
    v.drawCounter(mineCounterX, counterY, b.getRemainingMines());

    /* Bouton Emoji */
    v.drawEmoji(emojiX, emojiY, emojiState);

    /* Compteur Chronomètre */
    v.drawCounter(timerCounterX, counterY, elapsedSeconds);
}

void MinesApp::drawSingleCell(int gx, int gy) {
    Cell &c = b.get(gx, gy);
    int px = gridX + gx * 16;
    int py = gridY + gy * 16;

    if (!c.isRevealed) {
        if (c.isFlagged) {
            if (c.isFalseMine) {
                v.drawCell(px, py, SPR_CELL_FALSE_MINE);
            } else {
                v.drawCell(px, py, SPR_CELL_FLAG);
            }
        } else {
            v.drawCell(px, py, SPR_CELL_UNREVEALED);
        }
    } else {
        if (c.isMine) {
            if (c.isExploded) {
                v.drawCell(px, py, SPR_CELL_EXPLODED);
            } else {
                v.drawCell(px, py, SPR_CELL_MINE);
            }
        } else if (c.count > 0) {
            v.drawNumberCell(px, py, c.count);
        } else {
            v.drawCell(px, py, SPR_CELL_EMPTY);
        }
    }
}

void MinesApp::drawGrid() {
    for (int y = 0; y < b.getH(); y++) {
        for (int x = 0; x < b.getW(); x++) {
            drawSingleCell(x, y);
        }
    }
}

void MinesApp::menu() {
    state = STATE_MENU;
    cleardevice();

    /* Cadre de sélection avec fond plein texturé */
    v.drawPanel(190, 130, 260, 210, 1);
    v.setTextColor();
    outtextxy(250, 150, "DEMINEUR DOS");

    v.drawPanel(210, 180, 220, 32, 1);
    v.setTextColor();
    outtextxy(230, 192, "1. DEBUTANT (9x9)");

    v.drawPanel(210, 225, 220, 32, 1);
    v.setTextColor();
    outtextxy(230, 237, "2. INTERMEDIAIRE (16x16)");

    v.drawPanel(210, 270, 220, 32, 1);
    v.setTextColor();
    outtextxy(230, 282, "3. AVANCE (30x16)");

    m.show();
    int mx, my, mb;
    int lastB = 0;

    while (state == STATE_MENU) {
        if (kbhit()) {
            char ch = getch();
            if (ch == '1') play(BEGINNER);
            else if (ch == '2') play(INTERMEDIATE);
            else if (ch == '3') play(ADVANCED);
            else if (ch == 27) return; /* Echap pour quitter */
        }

        m.getStatus(mx, my, mb);
        if ((mb & 1) && !(lastB & 1)) {
            if (mx >= 210 && mx <= 430) {
                if (my >= 180 && my <= 212) play(BEGINNER);
                else if (my >= 225 && my <= 257) play(INTERMEDIATE);
                else if (my >= 270 && my <= 302) play(ADVANCED);
            }
        }
        lastB = mb;
    }
}

void MinesApp::play(Difficulty d) {
    m.hide();
    b.setup(d);
    computeLayout();

    state = STATE_PLAYING;
    emojiState = EMOJI_NORMAL;
    elapsedSeconds = 0;
    gameStartTime = 0;

    drawFullInterface();
    m.show();

    int mx, my, mb;
    int lastB = 0;
    time_t lastTick = 0;

    while (state == STATE_PLAYING) {
        /* Gestion du chronomètre */
        if (b.isStarted()) {
            time_t now = time(NULL);
            if (gameStartTime == 0) {
                gameStartTime = now;
                lastTick = now;
            }
            if (now != lastTick) {
                elapsedSeconds = (int)(now - gameStartTime);
                if (elapsedSeconds > 999) elapsedSeconds = 999;
                lastTick = now;
                m.hide();
                v.drawCounter(timerCounterX, counterY, elapsedSeconds);
                m.show();
            }
        }

        /* Touche clavier pour revenir au menu */
        if (kbhit()) {
            char k = getch();
            if (k == 27) { /* Echap */
                menu();
                return;
            }
        }

        m.getStatus(mx, my, mb);

        /* Visage surpris lorsque l'on maintient le clic sur la grille */
        int gx = (mx - gridX) / 16;
        int gy = (my - gridY) / 16;
        int onGrid = (gx >= 0 && gx < b.getW() && gy >= 0 && gy < b.getH());
        int onEmoji = (mx >= emojiX && mx <= emojiX + 24 && my >= emojiY && my <= emojiY + 24);

        int targetEmoji = EMOJI_NORMAL;
        if (onEmoji && (mb & 1)) {
            targetEmoji = EMOJI_CLICKED;
        } else if (onGrid && (mb & 1)) {
            targetEmoji = EMOJI_SURPRISE;
        }

        if (targetEmoji != emojiState) {
            emojiState = targetEmoji;
            m.hide();
            v.drawEmoji(emojiX, emojiY, emojiState);
            m.show();
        }

        /* Clic sur le bouton Emoji (Reset) */
        if (!(mb & 1) && (lastB & 1) && onEmoji) {
            m.hide();
            b.setup(d);
            emojiState = EMOJI_NORMAL;
            elapsedSeconds = 0;
            gameStartTime = 0;
            drawFullInterface();
            m.show();
            lastB = mb;
            continue;
        }

        /* Interaction sur la grille */
        if (onGrid) {
            /* 1. Détection Chord (clic gauche + droit ensemble) */
            if ((mb & 1) && (mb & 2) && !(lastB & 1 && lastB & 2)) {
                int exploded = 0;
                m.hide();
                b.chord(gx, gy, exploded);
                if (exploded) {
                    state = STATE_LOST;
                } else if (b.checkVictory()) {
                    state = STATE_WON;
                }
                drawGrid();
                v.drawCounter(mineCounterX, counterY, b.getRemainingMines());
                m.show();
            }
            /* 2. Clic gauche simple (Révélation) */
            else if ((mb & 1) && !(lastB & 1) && !(mb & 2)) {
                m.hide();
                int res = b.reveal(gx, gy);
                if (res == 0) {
                    state = STATE_LOST;
                } else if (b.checkVictory()) {
                    state = STATE_WON;
                }
                drawGrid();
                m.show();
            }
            /* 3. Clic droit simple (Drapeau) */
            else if ((mb & 2) && !(lastB & 2) && !(mb & 1)) {
                m.hide();
                b.toggleFlag(gx, gy);
                drawSingleCell(gx, gy);
                v.drawCounter(mineCounterX, counterY, b.getRemainingMines());
                m.show();
            }
        }

        lastB = mb;
    }

    /* Fin de partie */
    m.hide();
    if (state == STATE_LOST) {
        emojiState = EMOJI_LOST;
        b.revealAllMines();
        drawGrid();
        v.drawEmoji(emojiX, emojiY, emojiState);
    } else if (state == STATE_WON) {
        emojiState = EMOJI_WON;
        v.drawCounter(mineCounterX, counterY, 0);
        v.drawEmoji(emojiX, emojiY, emojiState);
    }
    m.show();

    /* Attente clic sur smiley pour recommencer ou touche pour le menu */
    while (state != STATE_PLAYING) {
        if (kbhit()) {
            getch();
            menu();
            return;
        }
        m.getStatus(mx, my, mb);
        if ((mb & 1) && !(lastB & 1)) {
            if (mx >= emojiX && mx <= emojiX + 24 && my >= emojiY && my <= emojiY + 24) {
                play(d);
                return;
            }
        }
        lastB = mb;
    }
}
