#include <graphics.h>
#include <dos.h>
#include <stdio.h>
#include <conio.h>
#include "minesapp.h"
#include "assetdef.h"

MinesApp::MinesApp() : state(STATE_MENU), emojiState(EMOJI_NORMAL), questionMarksEnabled(1) {
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

    /* 3. Chargement de la sprite sheet */
    int loaded = v.loadSprites("asset.bmp") ||
                 v.loadSprites("asset\\asset.bmp") ||
                 v.loadSprites("asset/asset.bmp") ||
                 v.loadSprites("minesweepersprite.bmp") ||
                 v.loadSprites("asset\\minesweepersprite.bmp");

    if (!loaded) {
        v.loadSpritesFromMemory(default_asset_bmp, sizeof(default_asset_bmp));
    }

    /* Boucle de jeu itérative (ZÉRO récursion) */
    while (1) {
        int choice = menu();
        if (choice < 0) {
            break; /* Echap dans le menu : quitter proprement */
        }
        play((Difficulty)choice);
    }
}

void MinesApp::computeLayout() {
    boardWidthPx  = b.getW() * 16;
    boardHeightPx = b.getH() * 16;

    /* Centrage horizontal et vertical dans l'écran standard VGA 640x480 */
    gridX = (640 - boardWidthPx) / 2;
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

    /* Grille en relief creusé encadrant pile les cellules de gridX à gridX + boardWidthPx - 1 */
    v.drawBezel(gridX - 1, gridY - 1, boardWidthPx + 1, boardHeightPx + 1, 0);

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

void MinesApp::drawSingleCell(int gx, int gy, int isDepressed) {
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
        } else if (c.isQuestion) {
            if (isDepressed) {
                v.drawCell(px, py, SPR_CELL_Q_CLICKED);
            } else {
                v.drawCell(px, py, SPR_CELL_QUESTION);
            }
        } else {
            if (isDepressed) {
                v.drawCell(px, py, SPR_CELL_EMPTY);
            } else {
                v.drawCell(px, py, SPR_CELL_UNREVEALED);
            }
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

int MinesApp::menu() {
    state = STATE_MENU;
    cleardevice();

    int screenH = 480;
    int panelW = 260, panelH = 260;
    int panelX = (640 - panelW) / 2;
    int panelY = (screenH - panelH) / 2;
    if (panelY < 10) panelY = 10;

    /* 1. Cadre de sélection avec fond plein texturé */
    v.drawPanel(panelX, panelY, panelW, panelH, 1);
    v.setTextColor();

    /* 2. Titre "DEMINEUR" en police vectorielle Sans-Serif, plus grand et parfaitement centré */
    settextstyle(SANS_SERIF_FONT, HORIZ_DIR, 3);
    const char* title = "DEMINEUR";
    int titleX = panelX + (panelW - textwidth((char*)title)) / 2;
    outtextxy(titleX, panelY + 15, (char*)title);
    outtextxy(titleX + 1, panelY + 15, (char*)title);
    outtextxy(titleX, panelY + 16, (char*)title);

    /* 3. Boutons en police standard avec centrage vertical */
    settextstyle(DEFAULT_FONT, HORIZ_DIR, 1);

    int btnX = panelX + 20;
    int btnW = 220;
    int btn1Y = panelY + 60;
    int btn2Y = panelY + 102;
    int btn3Y = panelY + 144;
    int optY  = panelY + 188;

    v.drawPanel(btnX, btn1Y, btnW, 32, 1);
    v.setTextColor();
    outtextxy(btnX + 20, btn1Y + 12, "1. DEBUTANT (9x9)");

    v.drawPanel(btnX, btn2Y, btnW, 32, 1);
    v.setTextColor();
    outtextxy(btnX + 20, btn2Y + 12, "2. INTERMEDIAIRE (16x16)");

    v.drawPanel(btnX, btn3Y, btnW, 32, 1);
    v.setTextColor();
    outtextxy(btnX + 20, btn3Y + 12, "3. AVANCE (30x16)");

    /* 4. Bouton d'option toggle pour les marques (?) */
    v.drawPanel(btnX, optY, btnW, 28, questionMarksEnabled ? 0 : 1);
    v.setTextColor();
    if (questionMarksEnabled) {
        outtextxy(btnX + 12, optY + 10, "[X] MARQUES (?)  (M)");
    } else {
        outtextxy(btnX + 12, optY + 10, "[ ] MARQUES (?)  (M)");
    }

    /* 5. Mentions de crédits en petite police (SMALL_FONT), discrètes dans les coins inférieurs */
    settextstyle(SMALL_FONT, HORIZ_DIR, 4);
    v.setCreditColor();
    int credY = screenH - 20;
    outtextxy(15, credY, "(C) Telev");

    const char* credRight = "Sprites: Black Squirrel";
    outtextxy(625 - textwidth((char*)credRight), credY, (char*)credRight);

    /* Rétablir la police par défaut pour le reste du jeu */
    settextstyle(DEFAULT_FONT, HORIZ_DIR, 1);

    m.show();
    int mx, my, mb;
    int lastB = 0;
    int pressedBtn = 0; /* 0=aucun, 1=debutant, 2=inter, 3=avance, 4=option */

    while (state == STATE_MENU) {
        if (kbhit()) {
            char ch = getch();
            if (ch == '1') return (int)BEGINNER;
            else if (ch == '2') return (int)INTERMEDIATE;
            else if (ch == '3') return (int)ADVANCED;
            else if (ch == 'm' || ch == 'M' || ch == '?') {
                questionMarksEnabled = !questionMarksEnabled;
                m.hide();
                v.drawPanel(btnX, optY, btnW, 28, questionMarksEnabled ? 0 : 1);
                v.setTextColor();
                outtextxy(btnX + 12, optY + 10, questionMarksEnabled ? "[X] MARQUES (?)  (M)" : "[ ] MARQUES (?)  (M)");
                m.show();
            }
            else if (ch == 27) return -1; /* Echap pour quitter */
        }

        m.getStatus(mx, my, mb);

        /* Détection Mouse-Down : on note quel bouton commence à être cliqué */
        if ((mb & 1) && !(lastB & 1)) {
            if (mx >= btnX && mx <= btnX + btnW) {
                if (my >= btn1Y && my <= btn1Y + 32) pressedBtn = 1;
                else if (my >= btn2Y && my <= btn2Y + 32) pressedBtn = 2;
                else if (my >= btn3Y && my <= btn3Y + 32) pressedBtn = 3;
                else if (my >= optY  && my <= optY + 28)  pressedBtn = 4;
                else pressedBtn = 0;
            } else {
                pressedBtn = 0;
            }
        }

        /* Détection Mouse-Up : l'action ne s'exécute QUE si le curseur est TOUJOURS sur le même bouton */
        if (!(mb & 1) && (lastB & 1)) {
            if (pressedBtn != 0 && (mx >= btnX && mx <= btnX + btnW)) {
                if (pressedBtn == 1 && (my >= btn1Y && my <= btn1Y + 32)) {
                    return (int)BEGINNER;
                } else if (pressedBtn == 2 && (my >= btn2Y && my <= btn2Y + 32)) {
                    return (int)INTERMEDIATE;
                } else if (pressedBtn == 3 && (my >= btn3Y && my <= btn3Y + 32)) {
                    return (int)ADVANCED;
                } else if (pressedBtn == 4 && (my >= optY && my <= optY + 28)) {
                    questionMarksEnabled = !questionMarksEnabled;
                    m.hide();
                    v.drawPanel(btnX, optY, btnW, 28, questionMarksEnabled ? 0 : 1);
                    v.setTextColor();
                    outtextxy(btnX + 12, optY + 10, questionMarksEnabled ? "[X] MARQUES (?)  (M)" : "[ ] MARQUES (?)  (M)");
                    m.show();
                }
            }
            pressedBtn = 0;
        }

        lastB = mb;
    }
    return -1;
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
    int pressedCellX = -1, pressedCellY = -1;
    int pressedOnEmoji = 0;

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
                m.hide();
                return; /* Quitte play() et retourne directement à la boucle itérative dans run() */
            }
        }

        m.getStatus(mx, my, mb);

        int gx = (mx - gridX) / 16;
        int gy = (my - gridY) / 16;
        int onGrid = (gx >= 0 && gx < b.getW() && gy >= 0 && gy < b.getH());
        int onEmoji = (mx >= emojiX && mx <= emojiX + 24 && my >= emojiY && my <= emojiY + 24);

        /* Visage de l'émoji selon l'état souris */
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

        /* 1. Détection Mouse-Down : on mémorise la case ou l'émoji cliqué */
        if ((mb & 1) && !(lastB & 1)) {
            if (onEmoji) {
                pressedOnEmoji = 1;
                pressedCellX = -1;
                pressedCellY = -1;
            } else if (onGrid) {
                pressedCellX = gx;
                pressedCellY = gy;
                pressedOnEmoji = 0;
                /* Afficher la case en état appuyé */
                m.hide();
                drawSingleCell(gx, gy, 1);
                m.show();
            } else {
                pressedCellX = -1;
                pressedCellY = -1;
                pressedOnEmoji = 0;
            }
        }

        /* Suivi du déplacement de la souris pendant que le bouton gauche est maintenu */
        if ((mb & 1) && (pressedCellX >= 0 || pressedCellY >= 0)) {
            /* Si le curseur a bougé sur une autre case ou hors grille, on restaure la case enfoncée */
            if (!onGrid || gx != pressedCellX || gy != pressedCellY) {
                m.hide();
                drawSingleCell(pressedCellX, pressedCellY, 0);
                m.show();
                pressedCellX = -1;
                pressedCellY = -1;
            }
        }

        /* 2. Clic droit simple (Drapeau / ?) au Mouse-Down */
        if ((mb & 2) && !(lastB & 2) && !(mb & 1) && onGrid) {
            m.hide();
            b.toggleFlag(gx, gy, questionMarksEnabled);
            drawSingleCell(gx, gy);
            v.drawCounter(mineCounterX, counterY, b.getRemainingMines());
            m.show();
        }

        /* 3. Chord (clic gauche + droit simultanés) */
        if ((mb & 1) && (mb & 2) && !(lastB & 1 && lastB & 2) && onGrid) {
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
            pressedCellX = -1;
            pressedCellY = -1;
            pressedOnEmoji = 0;
        }

        /* 4. Détection Mouse-Up (Relâchement du clic gauche) */
        if (!(mb & 1) && (lastB & 1)) {
            /* Bouton Emoji : réinitialise la partie uniquement si relâché DESSUS */
            if (pressedOnEmoji && onEmoji) {
                m.hide();
                b.setup(d);
                emojiState = EMOJI_NORMAL;
                elapsedSeconds = 0;
                gameStartTime = 0;
                drawFullInterface();
                m.show();
            }
            /* Grille : révèle la case uniquement si le curseur est TOUJOURS sur la même case */
            else if (pressedCellX >= 0 && pressedCellY >= 0 && onGrid) {
                if (gx == pressedCellX && gy == pressedCellY) {
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
            }
            pressedCellX = -1;
            pressedCellY = -1;
            pressedOnEmoji = 0;
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
            m.hide();
            return; /* Quitte play() et retourne directement à la boucle itérative dans run() */
        }
        m.getStatus(mx, my, mb);

        /* Détection Mouse-Down sur le smiley en fin de partie */
        int onEmojiEnd = (mx >= emojiX && mx <= emojiX + 24 && my >= emojiY && my <= emojiY + 24);
        if ((mb & 1) && !(lastB & 1)) {
            if (onEmojiEnd) {
                pressedOnEmoji = 1;
            }
        }

        /* Détection Mouse-Up sur le smiley pour relancer une partie */
        if (!(mb & 1) && (lastB & 1)) {
            if (pressedOnEmoji && onEmojiEnd) {
                play(d);
                return;
            }
            pressedOnEmoji = 0;
        }
        lastB = mb;
    }
}
