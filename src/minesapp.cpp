#include <graphics.h>
#include <dos.h>
#include <stdio.h>
#include <conio.h>
#include "minesapp.h"
#include "assetdef.h"

MinesApp::MinesApp() : state(STATE_MENU), emojiState(EMOJI_NORMAL), questionMarksEnabled(1) {
}

void MinesApp::run() {
     /* 1. Vérification matérielle VGA / EGA */
    if (!v.init()) {
        printf("\n%s\n",  STR_ERR_GRAPH_LINE1);
        printf("%s\n",    STR_ERR_GRAPH_LINE2);
        printf("%s\n",    STR_ERR_GRAPH_LINE3);
        printf("%s\n",    STR_ERR_GRAPH_LINE4);
        printf("%s\n\n",  STR_ERR_GRAPH_LINE1);
        return;
    }

    /* 2. Initialisation de la souris */
    if (!m.init()) {
        closegraph();
        printf("\n%s\n", STR_ERR_MOUSE);
        return;
    }

    /* 3. Chargement de la sprite sheet */
    int loaded = v.loadSprites("asset.bmp");

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
    gridY = ((v.getIsVGA() == 1 ? 480 : 350) - boardHeightPx) / 2 + 25;

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

    int screenH = (v.getIsVGA() == 1 ? 480 : 350);
    int panelW = 260, panelH = 296;
    int panelX = (640 - panelW) / 2;
    int panelY = (screenH - panelH) / 2;
    if (panelY < 10) panelY = 10;

    /* 1. Cadre de sélection avec fond plein texturé */
    v.drawPanel(panelX, panelY, panelW, panelH, 1);
    v.setTextColor();

    /* 2. Titre "DEMINEUR" en police vectorielle Sans-Serif, plus grand et parfaitement centré */
    settextstyle(SANS_SERIF_FONT, HORIZ_DIR, 3);
    int titleX = panelX + (panelW - textwidth((char*)STR_MENU_TITLE)) / 2;
    outtextxy(titleX, panelY + 15, (char*)STR_MENU_TITLE);
    outtextxy(titleX + 1, panelY + 15, (char*)STR_MENU_TITLE);
    outtextxy(titleX, panelY + 16, (char*)STR_MENU_TITLE);

    /* 3. Boutons en police standard avec centrage vertical */
    settextstyle(DEFAULT_FONT, HORIZ_DIR, 1);

    int btnX = panelX + 20;
    int btnW = 220;
    int btn1Y = panelY + 56;
    int btn2Y = panelY + 94;
    int btn3Y = panelY + 132;
    int optY  = panelY + 172;
    int scrY  = panelY + 208;

    v.drawPanel(btnX, btn1Y, btnW, 32, 1);
    v.setTextColor();
    outtextxy(btnX + 20, btn1Y + 12, (char*)STR_MENU_BEGINNER);

    v.drawPanel(btnX, btn2Y, btnW, 32, 1);
    v.setTextColor();
    outtextxy(btnX + 20, btn2Y + 12, (char*)STR_MENU_INTERMED);

    v.drawPanel(btnX, btn3Y, btnW, 32, 1);
    v.setTextColor();
    outtextxy(btnX + 20, btn3Y + 12, (char*)STR_MENU_ADVANCED);

    /* 4. Bouton d'option toggle pour les marques (?) */
    v.drawPanel(btnX, optY, btnW, 28, questionMarksEnabled ? 0 : 1);
    v.setTextColor();
    if (questionMarksEnabled) {
        outtextxy(btnX + 12, optY + 10, (char*)STR_MENU_MARKS_ON);
    } else {
        outtextxy(btnX + 12, optY + 10, (char*)STR_MENU_MARKS_OFF);
    }

    /* 5. Bouton "MEILLEURS TEMPS" */
    v.drawPanel(btnX, scrY, btnW, 28, 1);
    v.setTextColor();
    outtextxy(btnX + 24, scrY + 10, (char*)STR_MENU_SCORES);

    /* 6. Mentions de crédits en petite police (SMALL_FONT), discrètes dans les coins inférieurs */
    settextstyle(SMALL_FONT, HORIZ_DIR, 4);
    v.setCreditColor();
    int credY = screenH - 20;
    outtextxy(15, credY, (char*)STR_CREDIT_LEFT);

    outtextxy(625 - textwidth((char*)STR_CREDIT_RIGHT), credY, (char*)STR_CREDIT_RIGHT);


    /* Rétablir la police par défaut pour le reste du jeu */
    settextstyle(DEFAULT_FONT, HORIZ_DIR, 1);

    m.show();
    int mx, my, mb;
    int lastB = 0;
    int pressedBtn = 0; /* 0=aucun, 1=debutant, 2=inter, 3=avance, 4=option, 5=scores */

    while (state == STATE_MENU) {
        if (kbhit()) {
            char ch = getch();
            if (ch == '1') return (int)BEGINNER;
            else if (ch == '2') return (int)INTERMEDIATE;
            else if (ch == '3') return (int)ADVANCED;
            else if (ch == 't' || ch == 'T' || ch == 's' || ch == 'S') {
                m.hide();
                showHighScores();
                return menu();
            }
            else if (ch == 'm' || ch == 'M' || ch == '?') {
                questionMarksEnabled = !questionMarksEnabled;
                m.hide();
                v.drawPanel(btnX, optY, btnW, 28, questionMarksEnabled ? 0 : 1);
                v.setTextColor();
                outtextxy(btnX + 12, optY + 10, questionMarksEnabled ? (char*)STR_MENU_MARKS_ON : (char*)STR_MENU_MARKS_OFF);
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
                else if (my >= scrY  && my <= scrY + 28)  pressedBtn = 5;
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
                    outtextxy(btnX + 12, optY + 10, questionMarksEnabled ? (char*)STR_MENU_MARKS_ON : (char*)STR_MENU_MARKS_OFF);
                    m.show();
                } else if (pressedBtn == 5 && (my >= scrY && my <= scrY + 28)) {
                    m.hide();
                    showHighScores();
                    return menu();
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
    int pressedCellX = -1, pressedCellY = -1;   // Case ancrée au Mouse-Down (Clic gauche)
    int rightPressedX = -1, rightPressedY = -1; // Case ancrée au Mouse-Down (Clic droit)
    int chordAnchorX = -1, chordAnchorY = -1;   // Case ancrée au Mouse-Down (Chord)
    int isChording = 0;                         // 1 si le mode Chord est actif
    int pressedOnEmoji = 0;

    while (state == STATE_PLAYING) {
        /* -------------------------------------------------------------
           Gestion du chronomètre
           ------------------------------------------------------------- */
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

        /* -------------------------------------------------------------
           Touche clavier pour revenir au menu
           ------------------------------------------------------------- */
        if (kbhit()) {
            char k = getch();
            if (k == 27) { /* Echap */
                m.hide();
                return;
            }
        }

        /* -------------------------------------------------------------
           Lecture de l'état de la souris
           ------------------------------------------------------------- */
        m.getStatus(mx, my, mb);

        int gx = (mx - gridX) / 16;
        int gy = (my - gridY) / 16;
        int onGrid = (gx >= 0 && gx < b.getW() && gy >= 0 && gy < b.getH());
        int onEmoji = (mx >= emojiX && mx <= emojiX + 24 && my >= emojiY && my <= emojiY + 24);

        /* -------------------------------------------------------------
           Animation dynamique du visage de l'émoji (En cours de partie)
           ------------------------------------------------------------- */
        int targetEmoji = EMOJI_NORMAL;
        if (onEmoji && (mb & 1)) {
            /* L'émoji s'enfonce si on le survole avec le clic gauche */
            targetEmoji = EMOJI_CLICKED;
        } else if (pressedOnEmoji == 1) {
            /* Si le clic a commencé sur l'émoji mais qu'on glisse en dehors, 
               il redevient simplement NORMAL (pas de surprise) */
            targetEmoji = EMOJI_NORMAL;
        } else if (isChording || (onGrid && (mb & 1))) {
            /* Le mode surprise ne s'active QUE si le clic gauche/chord a commencé sur la grille */
            targetEmoji = EMOJI_SURPRISE;
        } else {
            targetEmoji = EMOJI_NORMAL;
        }

        if (targetEmoji != emojiState) {
            emojiState = targetEmoji;
            m.hide();
            v.drawEmoji(emojiX, emojiY, emojiState);
            m.show();
        }

        /* =========================================================================
           1. GESTION DU MOUSE-DOWN (Détection initiale du clic)
           ========================================================================= */
        if (((mb & 1) || (mb & 2)) && !(lastB & 1 || lastB & 2)) {
            if (onEmoji && (mb & 1)) {
                pressedOnEmoji = 1;
            } 
            else if (onGrid) {
                if ((mb & 1) && (mb & 2)) {
                    isChording = 1;
                    chordAnchorX = gx;
                    chordAnchorY = gy;
                } else if (mb & 1) {
                    pressedCellX = gx;
                    pressedCellY = gy;
                } else if (mb & 2) {
                    rightPressedX = gx;
                    rightPressedY = gy;
                }
            }
        }
        else if ((mb & 1) && (mb & 2) && !(lastB & 1 && lastB & 2) && onGrid && !isChording) {
            if (pressedCellX >= 0 && pressedCellY >= 0) {
                m.hide();
                drawSingleCell(pressedCellX, pressedCellY, 0);
                m.show();
            }
            isChording = 1;
            chordAnchorX = gx;
            chordAnchorY = gy;
            pressedCellX = -1;
            pressedCellY = -1;
        }

        /* =========================================================================
           2. GESTION DU SURVOL DYNAMIQUE (Focus Anti-clignotement)
           ========================================================================= */
        if (isChording) {
            if (onGrid && gx == chordAnchorX && gy == chordAnchorY) {
                if (pressedCellX != 1) { 
                    m.hide();
                    for (int dy = -1; dy <= 1; dy++) {
                        for (int dx = -1; dx <= 1; dx++) {
                            int nx = chordAnchorX + dx;
                            int ny = chordAnchorY + dy;
                            if (nx >= 0 && nx < b.getW() && ny >= 0 && ny < b.getH()) {
                                Cell &adj = b.get(nx, ny);
                                if (!adj.isRevealed && !adj.isFlagged) drawSingleCell(nx, ny, 1);
                            }
                        }
                    }
                    m.show();
                    pressedCellX = 1;
                }
            } else {
                if (pressedCellX == 1) {
                    m.hide();
                    for (int dy = -1; dy <= 1; dy++) {
                        for (int dx = -1; dx <= 1; dx++) {
                            int nx = chordAnchorX + dx;
                            int ny = chordAnchorY + dy;
                            if (nx >= 0 && nx < b.getW() && ny >= 0 && ny < b.getH()) drawSingleCell(nx, ny, 0);
                        }
                    }
                    m.show();
                    pressedCellX = 0;
                }
            }
        } 
        else if (pressedCellX >= 0 && pressedCellY >= 0) {
            if (onGrid && gx == pressedCellX && gy == pressedCellY) {
                if (rightPressedX != 1) {
                    m.hide();
                    drawSingleCell(pressedCellX, pressedCellY, 1);
                    m.show();
                    rightPressedX = 1;
                }
            } else {
                if (rightPressedX == 1) {
                    m.hide();
                    drawSingleCell(pressedCellX, pressedCellY, 0);
                    m.show();
                    rightPressedX = 0;
                }
            }
        }

        /* =========================================================================
           3. GESTION DU MOUSE-UP (Relâchement et exécution)
           ========================================================================= */
        if (isChording && (!(mb & 1) || !(mb & 2))) {
            m.hide();
            if (onGrid && gx == chordAnchorX && gy == chordAnchorY) {
                int exploded = 0;
                b.chord(chordAnchorX, chordAnchorY, exploded);
                if (exploded) state = STATE_LOST;
                else if (b.checkVictory()) state = STATE_WON;
                drawGrid();
                v.drawCounter(mineCounterX, counterY, b.getRemainingMines());
            } else {
                for (int dy = -1; dy <= 1; dy++) {
                    for (int dx = -1; dx <= 1; dx++) {
                        int nx = chordAnchorX + dx;
                        int ny = chordAnchorY + dy;
                        if (nx >= 0 && nx < b.getW() && ny >= 0 && ny < b.getH()) drawSingleCell(nx, ny, 0);
                    }
                }
            }
            isChording = 0; chordAnchorX = -1; chordAnchorY = -1; pressedCellX = -1; rightPressedX = -1;
            m.show();
        }
        else if (!(mb & 1) && (lastB & 1) && !isChording) {
            if (pressedOnEmoji && onEmoji) {
                m.hide(); b.setup(d); emojiState = EMOJI_NORMAL; elapsedSeconds = 0; gameStartTime = 0; drawFullInterface(); m.show();
                pressedOnEmoji = 0; pressedCellX = -1; rightPressedX = -1;
            }
            else if (pressedCellX >= 0 && pressedCellY >= 0) {
                if (onGrid && gx == pressedCellX && gy == pressedCellY) {
                    m.hide();
                    int res = b.reveal(gx, gy);
                    if (res == 0) state = STATE_LOST;
                    else if (b.checkVictory()) state = STATE_WON;
                    drawGrid();
                    m.show();
                } else {
                    m.hide(); drawSingleCell(pressedCellX, pressedCellY, 0); m.show();
                }
                pressedCellX = -1; pressedCellY = -1; rightPressedX = -1;
            }
            pressedOnEmoji = 0;
        }
        else if (!(mb & 2) && (lastB & 2) && !isChording) {
            if (rightPressedX >= 0 && rightPressedY >= 0 && pressedCellX == -1) {
                if (onGrid && gx == rightPressedX && gy == rightPressedY) {
                    m.hide();
                    b.toggleFlag(gx, gy, questionMarksEnabled);
                    drawSingleCell(gx, gy);
                    v.drawCounter(mineCounterX, counterY, b.getRemainingMines());
                    m.show();
                }
            }
            rightPressedX = -1; rightPressedY = -1;
        }

        lastB = mb;
    }

    /* Fin de partie : Écran fixe */
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

        /* Vérification d'un nouveau record */
        if (scores.isNewRecord(d, elapsedSeconds)) {
            promptNewRecord(d, elapsedSeconds);
            showHighScores();
            drawFullInterface();
        }
    }
    m.show();
    pressedOnEmoji = 0;

    /* Boucle finale d'attente avec comportement poussoir de l'émoji */
    while (state != STATE_PLAYING) {
        if (kbhit()) { getch(); m.hide(); return; }
        
        m.getStatus(mx, my, mb);
        int onEmojiEnd = (mx >= emojiX && mx <= emojiX + 24 && my >= emojiY && my <= emojiY + 24);

        int targetEmojiEnd = (state == STATE_WON) ? EMOJI_WON : EMOJI_LOST;
        if (onEmojiEnd && (mb & 1) && pressedOnEmoji) targetEmojiEnd = EMOJI_CLICKED;

        if (targetEmojiEnd != emojiState) {
            emojiState = targetEmojiEnd; m.hide(); v.drawEmoji(emojiX, emojiY, emojiState); m.show();
        }

        if ((mb & 1) && !(lastB & 1)) {
            if (onEmojiEnd) pressedOnEmoji = 1;
        }

        if (!(mb & 1) && (lastB & 1)) {
            if (pressedOnEmoji && onEmojiEnd) { pressedOnEmoji = 0; play(d); return; }
            pressedOnEmoji = 0;
        }
        lastB = mb;
    }
}

void MinesApp::showHighScores() {
    int screenH = (v.getIsVGA() == 1 ? 480 : 350);
    int pW = 320, pH = 210;
    int pX = (640 - pW) / 2;
    int pY = (screenH - pH) / 2;

    v.drawPanel(pX, pY, pW, pH, 1);
    v.setTextColor();

    settextstyle(SANS_SERIF_FONT, HORIZ_DIR, 2);
    const char* hTitle = STR_MENU_SCORES_TITLE;
    int tx = pX + (pW - textwidth((char*)hTitle)) / 2;
    outtextxy(tx, pY + 12, (char*)hTitle);

    settextstyle(DEFAULT_FONT, HORIZ_DIR, 1);
    const char* diffNames[3] = { STR_MENU_SCORES_BEGINNER, STR_MENU_SCORES_INTERMED, STR_MENU_SCORES_ADVANCED };
    char lineBuf[64];

    for (int i = 0; i < 3; i++) {
        const HighScore& s = scores.get((Difficulty)i);
        sprintf(lineBuf, "%-14s: %3d s  %-10s", diffNames[i], s.time, s.name);
        outtextxy(pX + 24, pY + 60 + i * 28, lineBuf);
    }

    int btnResetX = pX + 20, btnResetY = pY + 155, btnResetW = 130, btnH = 30;
    int btnOkX = pX + 175, btnOkY = pY + 155, btnOkW = 120;

    v.drawPanel(btnResetX, btnResetY, btnResetW, btnH, 1);
    v.setTextColor();
    outtextxy(btnResetX + 10, btnResetY + 11, STR_MENU_SCORES_RESET);

    v.drawPanel(btnOkX, btnOkY, btnOkW, btnH, 1);
    v.setTextColor();
    outtextxy(btnOkX + 48, btnOkY + 11, STR_MENU_SCORES_OK);

    m.show();
    int mx, my, mb, lastB = 0;
    int pressed = 0;

    while (1) {
        if (kbhit()) {
            char ch = getch();
            if (ch == 27 || ch == 13 || ch == 'o' || ch == 'O') break;
            if (ch == 'r' || ch == 'R') {
                scores.reset();
                scores.save();
                m.hide();
                showHighScores();
                return;
            }
        }

        m.getStatus(mx, my, mb);

        if ((mb & 1) && !(lastB & 1)) {
            if (my >= btnResetY && my <= btnResetY + btnH) {
                if (mx >= btnResetX && mx <= btnResetX + btnResetW) pressed = 1;
                else if (mx >= btnOkX && mx <= btnOkX + btnOkW) pressed = 2;
            }
        }

        if (!(mb & 1) && (lastB & 1)) {
            if (pressed == 1 && (mx >= btnResetX && mx <= btnResetX + btnResetW && my >= btnResetY && my <= btnResetY + btnH)) {
                scores.reset();
                scores.save();
                m.hide();
                showHighScores();
                return;
            } else if (pressed == 2 && (mx >= btnOkX && mx <= btnOkX + btnOkW && my >= btnOkY && my <= btnOkY + btnH)) {
                break;
            }
            pressed = 0;
        }

        lastB = mb;
    }
    m.hide();
}

void MinesApp::promptNewRecord(Difficulty d, int seconds) {
    int screenH = (v.getIsVGA() == 1 ? 480 : 350);
    int pW = 340, pH = 190;
    int pX = (640 - pW) / 2;
    int pY = (screenH - pH) / 2;

    v.drawPanel(pX, pY, pW, pH, 1);
    v.setTextColor();

    settextstyle(SANS_SERIF_FONT, HORIZ_DIR, 2);
    const char* hTitle = STR_MENU_SCORES_NEWRECORD1;
    int tx = pX + (pW - textwidth((char*)hTitle)) / 2;
    outtextxy(tx, pY + 12, (char*)hTitle);

    settextstyle(DEFAULT_FONT, HORIZ_DIR, 1);
    char buf[64];
    const char* diffNames[3] = { STR_MENU_SCORES_BEGINNER, STR_MENU_SCORES_INTERMED, STR_MENU_SCORES_ADVANCED };
    sprintf(buf, STR_MENU_SCORES_NEWRECORD2, diffNames[(int)d], seconds);
    outtextxy(pX + (pW - textwidth(buf)) / 2, pY + 50, buf);

    outtextxy(pX + 25, pY + 76, STR_MENU_SCORES_NAME);

    int editX = pX + 25, editY = pY + 96, editW = 290, editH = 24;
    v.drawSunkenRect(editX, editY, editW, editH);

    int btnOkX = pX + (pW - 100) / 2, btnOkY = pY + 138, btnOkW = 100, btnOkH = 28;
    v.drawPanel(btnOkX, btnOkY, btnOkW, btnOkH, 1);
    v.setTextColor();
    outtextxy(btnOkX + 40, btnOkY + 10, STR_MENU_SCORES_OK);

    char name[16];
    name[0] = '\0';
    int len = 0;

    m.show();
    int mx, my, mb, lastB = 0;
    int pressedOk = 0;

    while (1) {
        if (kbhit()) {
            char ch = getch();
            if (ch == 13) {
                break;
            } else if (ch == 27) {
                break;
            } else if (ch == 8) {
                if (len > 0) {
                    len--;
                    name[len] = '\0';
                }
            } else if (ch >= 32 && ch <= 126 && len < 15) {
                name[len++] = ch;
                name[len] = '\0';
            }

            m.hide();
            v.drawSunkenRect(editX, editY, editW, editH);
            setcolor(WHITE);
            char dispBuf[20];
            sprintf(dispBuf, "%s_", name);
            outtextxy(editX + 8, editY + 8, dispBuf);
            m.show();
        }

        m.getStatus(mx, my, mb);

        if ((mb & 1) && !(lastB & 1)) {
            if (mx >= btnOkX && mx <= btnOkX + btnOkW && my >= btnOkY && my <= btnOkY + btnOkH) {
                pressedOk = 1;
            }
        }

        if (!(mb & 1) && (lastB & 1)) {
            if (pressedOk && mx >= btnOkX && mx <= btnOkX + btnOkW && my >= btnOkY && my <= btnOkY + btnOkH) {
                break;
            }
            pressedOk = 0;
        }

        lastB = mb;
    }

    m.hide();
    if (len > 0) {
        scores.update(d, name, seconds);
    } else {
        scores.update(d, STR_MENU_SCORES_DEFAULT, seconds);
    }
}

