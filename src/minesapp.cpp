#include <graphics.h>
#include <dos.h>
#include <stdio.h>
#include <conio.h>
#include "minesapp.h"
#include "assetdef.h"

/* Lit une touche (a appeler uniquement si kbhit() est vrai).
   Retourne :  0..255  = caractere ordinaire
               256+n   = touche etendue, n = scan code (fleches, F1.., Alt+lettre...) */
static int readKey()
{
    int c = getch();
    if (c == 0 || c == 0xE0)
    {
        c = 256 + getch();
    }
    return c;
}

static void drawText(int x, int y, const char *s)
{
    outtextxy(x, y, (char *)s);
}

static int textW(const char *s)
{
    return textwidth((char *)s);
}

/* Rend la main au systeme hote quand il n'y a rien a faire.
   Appelee a chaque tour des boucles d'attente (souris / clavier). */
static void idle()
{
    union REGS r;

    /* 1. Cede la tranche de temps : Windows 3.x/9x, OS/2, DOSBox-X, DOSIDLE... */
    r.x.ax = 0x1680;
    int86(0x2F, &r, &r);

    /* 2. Arret du processeur jusqu'a la prochaine interruption
          (tick timer 18,2 Hz, souris, clavier) */
    __emit__(0xF4); /* HLT */
}

static void showMemoryError()
{
    cleardevice();
    settextstyle(DEFAULT_FONT, HORIZ_DIR, 1);
    setcolor(WHITE);
    drawText(20, 20, STR_ERR_MEMORY);
    drawText(20, 40, STR_ERR_PRESS_KEY);
    readKey();
}

MinesApp::MinesApp() : state(STATE_MENU), emojiState(EMOJI_NORMAL), questionMarksEnabled(1)
{
}

void MinesApp::run(const char *exePath)
{
    scores.init(exePath);

    /* 1. Verification materielle VGA / EGA */
    if (!v.init())
    {
        printf("\n%s\n", STR_ERR_GRAPH_LINE1);
        printf("%s\n", STR_ERR_GRAPH_LINE2);
        printf("%s\n", STR_ERR_GRAPH_LINE3);
        printf("%s\n", STR_ERR_GRAPH_LINE4);
        printf("%s\n\n", STR_ERR_GRAPH_LINE1);
        return;
    }

    /* 2. Initialisation de la souris */
    if (!m.init())
    {
        closegraph();
        printf("\n%s\n", STR_ERR_MOUSE);
        return;
    }

    /* 3. Chargement de la sprite sheet */
    int loaded = v.loadSprites("asset.bmp");
    if (!loaded)
    {
        loaded = v.loadSpritesFromMemory(default_asset_bmp, sizeof(default_asset_bmp));
    }
    if (!loaded)
    {
        closegraph();
        printf("\n%s\n", STR_ERR_SPRITES);
        return;
    }

    /* Boucle de jeu iterative (ZeRO recursion) */
    while (1)
    {
        int choice = menu();
        if (choice < 0)
        {
            break; /* Echap dans le menu : quitter proprement */
        }
        int again;
        do
        {
            again = play((Difficulty)choice);
        } while (again);
    }
}

void MinesApp::computeLayout()
{
    int cellH = (v.getIsVGA() == 1) ? 16 : 12;

    boardWidthPx = b.getW() * 16;
    boardHeightPx = b.getH() * cellH;

    /* Centrage horizontal et vertical */
    gridX = (640 - boardWidthPx) / 2;
    gridY = ((v.getIsVGA() == 1 ? 480 : 350) - boardHeightPx) / 2 + 25;

    headerW = boardWidthPx;
    headerH = (v.getIsVGA() == 1) ? 37 : 28;
    headerX = gridX;
    headerY = gridY - headerH - 6;

    emojiX = headerX + (headerW - 24) / 2;
    emojiY = headerY + (headerH - (v.getIsVGA() == 1 ? 24 : 18)) / 2;

    mineCounterX = headerX + 6;
    timerCounterX = headerX + headerW - (13 * 3) - 6;
    counterY = headerY + (headerH - (v.getIsVGA() == 1 ? 23 : 17)) / 2;
}

void MinesApp::drawFullInterface(int isInitial)
{
    cleardevice();

    /* Cadre principal englobant (look Windows 3.1) avec fond opaque */
    v.drawPanel(headerX - 6, headerY - 6, headerW + 12, (gridY + boardHeightPx + 6) - (headerY - 6), 1);

    /* En-tete */
    v.drawPanel(headerX, headerY, headerW, headerH, 0);

    /* Grille en relief creuse encadrant pile les cellules de gridX a gridX + boardWidthPx - 1 */
    v.drawBezel(gridX - 1, gridY - 1, boardWidthPx + 1, boardHeightPx + 1, 0);

    drawHeader();

    // Si c'est le debut d'une partie, on utilise le trace ultra-rapide
    if (isInitial)
    {
        drawEmptyGrid();
    }
    else
    {
        drawGrid();
    }
}

void MinesApp::drawHeader()
{
    /* Compteur de mines restantes */
    v.drawCounter(mineCounterX, counterY, b.getRemainingMines());

    /* Bouton Emoji */
    v.drawEmoji(emojiX, emojiY, emojiState);

    /* Compteur Chronometre */
    v.drawCounter(timerCounterX, counterY, elapsedSeconds);
}

void MinesApp::drawSingleCell(int gx, int gy, int isDepressed)
{
    int cellH = (v.getIsVGA() == 1) ? 16 : 12;
    Cell &c = b.get(gx, gy);
    int px = gridX + gx * 16;
    int py = gridY + gy * cellH;

    if (!c.isRevealed)
    {
        if (c.isFlagged)
        {
            if (c.isFalseMine)
            {
                v.drawCell(px, py, SPR_CELL_FALSE_MINE);
            }
            else
            {
                v.drawCell(px, py, SPR_CELL_FLAG);
            }
        }
        else if (c.isQuestion)
        {
            if (isDepressed)
            {
                v.drawCell(px, py, SPR_CELL_Q_CLICKED);
            }
            else
            {
                v.drawCell(px, py, SPR_CELL_QUESTION);
            }
        }
        else
        {
            if (isDepressed)
            {
                v.drawCell(px, py, SPR_CELL_EMPTY);
            }
            else
            {
                v.drawCell(px, py, SPR_CELL_UNREVEALED);
            }
        }
    }
    else
    {
        if (c.isMine)
        {
            if (c.isExploded)
            {
                v.drawCell(px, py, SPR_CELL_EXPLODED);
            }
            else
            {
                v.drawCell(px, py, SPR_CELL_MINE);
            }
        }
        else if (c.count > 0)
        {
            v.drawNumberCell(px, py, c.count);
        }
        else
        {
            v.drawCell(px, py, SPR_CELL_EMPTY);
        }
    }
}

void MinesApp::drawEmptyGrid()
{
    v.drawUnrevealedGrid(gridX, gridY, b.getW(), b.getH());
}

void MinesApp::drawGrid()
{
    for (int y = 0; y < b.getH(); y++)
    {
        for (int x = 0; x < b.getW(); x++)
        {
            drawSingleCell(x, y);
        }
    }
}

int MinesApp::menu()
{
redraw_menu:
    state = STATE_MENU;
    cleardevice();

    /* 1. Determination des hauteurs selon le mode graphique */
    int isVga = (v.getIsVGA() == 1);
    int screenH = isVga ? 480 : 350;

    /* Calcul dynamique des dimensions du panneau central */
    int panelW = 260;
    int panelH = isVga ? 296 : 216; /* Compense la hauteur du panneau principal */

    int panelX = (640 - panelW) / 2;
    int panelY = (screenH - panelH) / 2;
    if (panelY < 10)
        panelY = 10;

    /* Cadre de selection avec fond plein texture */
    v.drawPanel(panelX, panelY, panelW, panelH, 1);
    v.setTextColor();

    /* 2. Titre "DEMINEUR" parfaitement centre */
    if (isVga)
    {
        settextstyle(SANS_SERIF_FONT, HORIZ_DIR, 3);
    }
    else
    {
        settextstyle(SANS_SERIF_FONT, HORIZ_DIR, 2); /* Taille 2 plus fine en EGA */
    }

    int titleX = panelX + (panelW - textW(STR_MENU_TITLE)) / 2;
    int titleY = panelY + (isVga ? 15 : 12);
    drawText(titleX, titleY, STR_MENU_TITLE);
    drawText(titleX + 1, titleY, STR_MENU_TITLE);
    drawText(titleX, titleY + 1, STR_MENU_TITLE);

    /* 3. Boutons en police standard avec coordonnees verticales adaptees */
    settextstyle(DEFAULT_FONT, HORIZ_DIR, 1);

    int btnX = panelX + 20;
    int btnW = 220;

    /* Hauteur des boutons reduite en EGA (24px au lieu de 32px pour garder le ratio) */
    int btnH = isVga ? 32 : 24;
    /* Hauteur des options et meilleurs temps */
    int optH = isVga ? 28 : 21;

    /* Recalcul des positions Y pour eviter que les boutons ne se chevauchent */
    int btn1Y = panelY + (isVga ? 56 : 41);
    int btn2Y = btn1Y + btnH + (isVga ? 6 : 4);
    int btn3Y = btn2Y + btnH + (isVga ? 6 : 4);
    int optY = btn3Y + btnH + (isVga ? 8 : 6);
    int scrY = optY + optH + (isVga ? 8 : 6);

    /* Decalage vertical du texte a l'interieur des boutons pour un centrage parfait */
    int textPaddingBtn = isVga ? 12 : 8;
    int textPaddingOpt = isVga ? 10 : 6;

    /* Dessin du bouton 1 : Debutant */
    v.drawPanel(btnX, btn1Y, btnW, btnH, 1);
    v.setTextColor();
    drawText(btnX + 20, btn1Y + textPaddingBtn, STR_MENU_BEGINNER);

    /* Dessin du bouton 2 : Intermediaire */
    v.drawPanel(btnX, btn2Y, btnW, btnH, 1);
    v.setTextColor();
    drawText(btnX + 20, btn2Y + textPaddingBtn, STR_MENU_INTERMED);

    /* Dessin du bouton 3 : Avance */
    v.drawPanel(btnX, btn3Y, btnW, btnH, 1);
    v.setTextColor();
    drawText(btnX + 20, btn3Y + textPaddingBtn, STR_MENU_ADVANCED);

    /* 4. Bouton d'option toggle pour les marques (?) */
    v.drawPanel(btnX, optY, btnW, optH, questionMarksEnabled ? 0 : 1);
    v.setTextColor();
    if (questionMarksEnabled)
    {
        drawText(btnX + 12, optY + textPaddingOpt, STR_MENU_MARKS_ON);
    }
    else
    {
        drawText(btnX + 12, optY + textPaddingOpt, STR_MENU_MARKS_OFF);
    }

    /* 5. Bouton "MEILLEURS TEMPS" */
    v.drawPanel(btnX, scrY, btnW, optH, 1);
    v.setTextColor();
    drawText(btnX + 24, scrY + textPaddingOpt, STR_MENU_SCORES);

    /* 6. Mentions de credits (Coins inferieurs de l'ecran global) */
    settextstyle(SMALL_FONT, HORIZ_DIR, 4);
    v.setCreditColor();
    int credY = screenH - 20;
    drawText(15, credY, STR_CREDIT_LEFT);
    drawText(625 - textW(STR_CREDIT_RIGHT), credY, STR_CREDIT_RIGHT);

    /* Retablir la police par defaut pour le reste du jeu */
    settextstyle(DEFAULT_FONT, HORIZ_DIR, 1);

    m.show();
    int mx, my, mb;
    int lastB = 0;
    int pressedBtn = 0; /* 0=aucun, 1=debutant, 2=inter, 3=avance, 4=option, 5=scores */

    while (state == STATE_MENU)
    {
        if (kbhit())
        {
            int ch = readKey();
            if (ch == '1')
                return (int)BEGINNER;
            else if (ch == '2')
                return (int)INTERMEDIATE;
            else if (ch == '3')
                return (int)ADVANCED;
            else if (ch == 't' || ch == 'T' || ch == 's' || ch == 'S')
            {
                m.hide();
                showHighScores();
                goto redraw_menu;
            }
            else if (ch == 'm' || ch == 'M' || ch == '?')
            {
                questionMarksEnabled = !questionMarksEnabled;
                m.hide();
                v.drawPanel(btnX, optY, btnW, optH, questionMarksEnabled ? 0 : 1);
                v.setTextColor();
                drawText(btnX + 12, optY + textPaddingOpt, questionMarksEnabled ? STR_MENU_MARKS_ON : STR_MENU_MARKS_OFF);
                m.show();
            }
            else if (ch == 27)
                return -1;
        }

        m.getStatus(mx, my, mb);

        /* Detection Mouse-Down avec les nouvelles hauteurs dynamiques (btnH et optH) */
        if ((mb & 1) && !(lastB & 1))
        {
            if (mx >= btnX && mx <= btnX + btnW)
            {
                if (my >= btn1Y && my <= btn1Y + btnH)
                    pressedBtn = 1;
                else if (my >= btn2Y && my <= btn2Y + btnH)
                    pressedBtn = 2;
                else if (my >= btn3Y && my <= btn3Y + btnH)
                    pressedBtn = 3;
                else if (my >= optY && my <= optY + optH)
                    pressedBtn = 4;
                else if (my >= scrY && my <= scrY + optH)
                    pressedBtn = 5;
                else
                    pressedBtn = 0;
            }
            else
            {
                pressedBtn = 0;
            }
        }

        /* Detection Mouse-Up calee elle aussi sur les hauteurs dynamiques */
        if (!(mb & 1) && (lastB & 1))
        {
            if (pressedBtn != 0 && (mx >= btnX && mx <= btnX + btnW))
            {
                if (pressedBtn == 1 && (my >= btn1Y && my <= btn1Y + btnH))
                {
                    return (int)BEGINNER;
                }
                else if (pressedBtn == 2 && (my >= btn2Y && my <= btn2Y + btnH))
                {
                    return (int)INTERMEDIATE;
                }
                else if (pressedBtn == 3 && (my >= btn3Y && my <= btn3Y + btnH))
                {
                    return (int)ADVANCED;
                }
                else if (pressedBtn == 4 && (my >= optY && my <= optY + optH))
                {
                    questionMarksEnabled = !questionMarksEnabled;
                    m.hide();
                    v.drawPanel(btnX, optY, btnW, optH, questionMarksEnabled ? 0 : 1);
                    v.setTextColor();
                    drawText(btnX + 12, optY + textPaddingOpt, questionMarksEnabled ? STR_MENU_MARKS_ON : STR_MENU_MARKS_OFF);
                    m.show();
                }
                else if (pressedBtn == 5 && (my >= scrY && my <= scrY + optH))
                {
                    m.hide();
                    showHighScores();
                    goto redraw_menu;
                }
            }
            pressedBtn = 0;
        }

        lastB = mb;
        idle();
    }
    return -1;
}

int MinesApp::play(Difficulty d)
{
    m.hide();
    if (!b.setup(d))
    {
        showMemoryError();
        return 0; /* retour au menu */
    }
    computeLayout();

    state = STATE_PLAYING;
    emojiState = EMOJI_NORMAL;
    elapsedSeconds = 0;
    gameStartTime = 0;

    drawFullInterface(1);
    m.show();

    int mx, my, mb;
    int lastB = 0;
    time_t lastTick = 0;
    int pressedCellX = -1, pressedCellY = -1;   // Case ancree au Mouse-Down (Clic gauche)
    int rightPressedX = -1, rightPressedY = -1; // Case ancree au Mouse-Down (Clic droit)
    int isChording = 0;                         // 1 si le mode Chord est actif
    int pressedOnEmoji = 0;
    int lastChordX = -1;
    int lastChordY = -1;
    int lastLeftX = -1;
    int lastLeftY = -1;

    while (state == STATE_PLAYING)
    {
        /* -------------------------------------------------------------
           Gestion du chronometre
           ------------------------------------------------------------- */
        if (b.isStarted())
        {
            time_t now = time(NULL);
            if (gameStartTime == 0)
            {
                gameStartTime = now;
                lastTick = now;
            }
            if (now != lastTick)
            {
                elapsedSeconds = (int)(now - gameStartTime);
                if (elapsedSeconds > 999)
                    elapsedSeconds = 999;
                lastTick = now;
                m.hide();
                v.drawCounter(timerCounterX, counterY, elapsedSeconds);
                m.show();
            }
        }

        /* -------------------------------------------------------------
           Touche clavier pour revenir au menu
           ------------------------------------------------------------- */
        if (kbhit())
        {
            int k = readKey();
            if (k == 27)
            { /* Echap */
                m.hide();
                return 0;
            }
        }

        /* -------------------------------------------------------------
           Lecture de l'etat de la souris
           ------------------------------------------------------------- */
        m.getStatus(mx, my, mb);

        int cellH = (v.getIsVGA() == 1) ? 16 : 12;

        int gx = (mx >= gridX) ? (mx - gridX) / 16 : -1;
        int gy = (my >= gridY) ? (my - gridY) / cellH : -1;
        int onGrid = (gx >= 0 && gx < b.getW() && gy >= 0 && gy < b.getH());
        int emojiH = (v.getIsVGA() == 1) ? 24 : 18;
        int onEmoji = (mx >= emojiX && mx < emojiX + 24 && my >= emojiY && my < emojiY + emojiH);

        /* -------------------------------------------------------------
           Animation dynamique du visage de l'emoji (En cours de partie)
           ------------------------------------------------------------- */
        int targetEmoji = EMOJI_NORMAL;
        if (onEmoji && (mb & 1))
        {
            /* L'emoji s'enfonce si on le survole avec le clic gauche */
            targetEmoji = EMOJI_CLICKED;
        }
        else if (pressedOnEmoji == 1)
        {
            /* Si le clic a commence sur l'emoji mais qu'on glisse en dehors,
               il redevient simplement NORMAL (pas de surprise) */
            targetEmoji = EMOJI_NORMAL;
        }
        else if (isChording || (onGrid && (mb & 1)))
        {
            /* Le mode surprise ne s'active QUE si le clic gauche/chord a commence sur la grille */
            targetEmoji = EMOJI_SURPRISE;
        }
        else
        {
            targetEmoji = EMOJI_NORMAL;
        }

        if (targetEmoji != emojiState)
        {
            emojiState = targetEmoji;
            m.hide();
            v.drawEmoji(emojiX, emojiY, emojiState);
            m.show();
        }

        /* =========================================================================
           1. GESTION DU MOUSE-DOWN (Detection initiale du clic)
           ========================================================================= */
        if (((mb & 1) || (mb & 2)) && !(lastB & 1 || lastB & 2))
        {
            if (onEmoji && (mb & 1))
            {
                pressedOnEmoji = 1;
            }
            else if (onGrid)
            {
                if ((mb & 1) && (mb & 2))
                {
                    isChording = 1;
                }
                else if (mb & 1)
                {
                    pressedCellX = gx;
                    pressedCellY = gy;
                }
                else if (mb & 2)
                {
                    rightPressedX = gx;
                    rightPressedY = gy;
                }
            }
        }
        else if ((mb & 1) && (mb & 2) && !(lastB & 1 && lastB & 2) && onGrid && !isChording)
        {
            if (pressedCellX >= 0 && pressedCellY >= 0)
            {
                m.hide();
                drawSingleCell(pressedCellX, pressedCellY, 0);
                m.show();
            }
            isChording = 1;
            pressedCellX = -1;
            pressedCellY = -1;
            lastLeftX = -1;
            lastLeftY = -1;
        }

        /* =========================================================================
        2. GESTION DU SURVOL DYNAMIQUE (Focus Anti-clignotement)
        ========================================================================= */
        if (isChording)
        {
            if (onGrid)
            {
                // OPTIMISATION : On ne redessine que si la souris a change de case
                if (lastChordX != gx || lastChordY != gy)
                {

                    m.hide();
                    // 1. Relever les anciennes cases enfoncees
                    if (lastChordX != -1 && lastChordY != -1)
                    {
                        for (int dy = -1; dy <= 1; dy++)
                        {
                            for (int dx = -1; dx <= 1; dx++)
                            {
                                int nx = lastChordX + dx;
                                int ny = lastChordY + dy;
                                if (nx >= 0 && nx < b.getW() && ny >= 0 && ny < b.getH())
                                {
                                    drawSingleCell(nx, ny, 0);
                                }
                            }
                        }
                    }

                    // 2. Enfoncer les 9 nouvelles cases autour de la souris actuelle (gx, gy)
                    for (int dy = -1; dy <= 1; dy++)
                    {
                        for (int dx = -1; dx <= 1; dx++)
                        {
                            int nx = gx + dx;
                            int ny = gy + dy;
                            if (nx >= 0 && nx < b.getW() && ny >= 0 && ny < b.getH())
                            {
                                Cell &adj = b.get(nx, ny);
                                if (!adj.isRevealed && !adj.isFlagged)
                                {
                                    drawSingleCell(nx, ny, 1);
                                }
                            }
                        }
                    }
                    m.show();

                    // On met a jour la position courante du Chord
                    lastChordX = gx;
                    lastChordY = gy;
                }
            }
            else
            {
                // Si la souris sort de la grille, on releve tout
                if (lastChordX != -1 && lastChordY != -1)
                {
                    m.hide();
                    for (int dy = -1; dy <= 1; dy++)
                    {
                        for (int dx = -1; dx <= 1; dx++)
                        {
                            int nx = lastChordX + dx;
                            int ny = lastChordY + dy;
                            if (nx >= 0 && nx < b.getW() && ny >= 0 && ny < b.getH())
                            {
                                drawSingleCell(nx, ny, 0);
                            }
                        }
                    }
                    m.show();

                    lastChordX = -1;
                    lastChordY = -1;
                }
            }
        }
        else if (pressedCellX >= 0 && pressedCellY >= 0)
        {
            // Si la souris est sur la grille et que le joueur maintient le clic gauche enfonce
            if (onGrid)
            {
                // OPTIMISATION : On ne redessine que si la souris a change de case
                if (lastLeftX != gx || lastLeftY != gy)
                {

                    m.hide();
                    // 1. Relever l'ancienne case qui avait ete enfoncee au cycle d'avant
                    if (lastLeftX != -1 && lastLeftY != -1)
                    {
                        drawSingleCell(lastLeftX, lastLeftY, 0);
                    }

                    // 2. Enfoncer visuellement la NOUVELLE case actuellement survolee
                    Cell &c = b.get(gx, gy);
                    if (!c.isRevealed && !c.isFlagged)
                    {
                        drawSingleCell(gx, gy, 1); // 1 = enfonce
                    }
                    m.show();

                    // On memorise la position actuelle
                    lastLeftX = gx;
                    lastLeftY = gy;
                }
            }
            else
            {
                // Si la souris sort de la grille pendant le clic maintenu, on releve la derniere case
                if (lastLeftX != -1 && lastLeftY != -1)
                {
                    m.hide();
                    drawSingleCell(lastLeftX, lastLeftY, 0);
                    m.show();
                    lastLeftX = -1;
                    lastLeftY = -1;
                }
            }
        }

        /* =========================================================================
        3. GESTION DU MOUSE-UP (Relachement et execution)
        ========================================================================= */
        if (isChording && (!(mb & 1) || !(mb & 2)))
        {
            m.hide();
            if (onGrid && gx == lastChordX && gy == lastChordY)
            {
                int exploded = 0;
                b.chord(lastChordX, lastChordY, exploded);

                if (exploded)
                {
                    state = STATE_LOST;
                    drawGrid();
                }
                else if (b.checkVictory())
                {
                    state = STATE_WON;
                    drawGrid();
                }
                else
                {
                    // 1. Redessiner les cases revelees par la cascade
                    renderDirtyCells();

                    // 2. Restaurer les cases 3x3 qui etaient visuellement enfoncees
                    for (int dy = -1; dy <= 1; dy++)
                    {
                        for (int dx = -1; dx <= 1; dx++)
                        {
                            int nx = lastChordX + dx;
                            int ny = lastChordY + dy;
                            if (nx >= 0 && nx < b.getW() && ny >= 0 && ny < b.getH())
                            {
                                drawSingleCell(nx, ny);
                            }
                        }
                    }
                }
                v.drawCounter(mineCounterX, counterY, b.getRemainingMines());
            }
            else
            {
                // Relachement hors case : relever la zone 3x3
                if (lastChordX != -1 && lastChordY != -1)
                {
                    for (int dy = -1; dy <= 1; dy++)
                    {
                        for (int dx = -1; dx <= 1; dx++)
                        {
                            int nx = lastChordX + dx;
                            int ny = lastChordY + dy;
                            if (nx >= 0 && nx < b.getW() && ny >= 0 && ny < b.getH())
                            {
                                drawSingleCell(nx, ny, 0);
                            }
                        }
                    }
                }
            }
            isChording = 0;
            pressedCellX = -1;
            pressedCellY = -1;
            rightPressedX = -1;
            rightPressedY = -1;
            lastChordX = -1;
            lastChordY = -1;
            lastLeftX = -1;
            lastLeftY = -1;
            m.show();
        }
        else if (!(mb & 1) && (lastB & 1) && !isChording)
        {
            if (pressedOnEmoji && onEmoji)
            {
                m.hide();
                if (!b.setup(d))
                {
                    showMemoryError();
                    return 0;
                }
                emojiState = EMOJI_NORMAL;
                elapsedSeconds = 0;
                gameStartTime = 0;
                drawFullInterface(1);
                m.show();
                pressedOnEmoji = 0;
                pressedCellX = -1;
                rightPressedX = -1;
            }
            else if (pressedCellX >= 0 && pressedCellY >= 0)
            {
                if (onGrid && gx == lastLeftX && gy == lastLeftY)
                {
                    b.clearDirty();
                    int res = b.reveal(lastLeftX, lastLeftY);

                    if (res == 0)
                    {
                        state = STATE_LOST;
                    }
                    else if (b.checkVictory())
                    {
                        state = STATE_WON;
                        m.hide();
                        drawGrid();
                        m.show();
                    }
                    else
                    {
                        renderDirtyCells();
                    }
                }
                else
                {
                    if (lastLeftX != -1 && lastLeftY != -1)
                    {
                        m.hide();
                        drawSingleCell(lastLeftX, lastLeftY, 0);
                        m.show();
                    }
                }
                pressedCellX = -1;
                pressedCellY = -1;
                rightPressedX = -1;
                lastLeftX = -1;
                lastLeftY = -1;
            }
            pressedOnEmoji = 0;
        }
        else if (!(mb & 2) && (lastB & 2) && !isChording)
        {
            if (rightPressedX >= 0 && rightPressedY >= 0 && pressedCellX == -1)
            {
                if (onGrid && gx == rightPressedX && gy == rightPressedY)
                {
                    b.toggleFlag(gx, gy, questionMarksEnabled);
                    renderDirtyCells();

                    m.hide();
                    v.drawCounter(mineCounterX, counterY, b.getRemainingMines());
                    m.show();
                }
            }
            rightPressedX = -1;
            rightPressedY = -1;
        }

        lastB = mb;
        idle();
    }

    /* Fin de partie : ecran fixe */
    m.hide();
    if (state == STATE_LOST)
    {
        emojiState = EMOJI_LOST;

        m.hide(); // On cache la souris une seule fois avant la grosse mise a jour

        // Au lieu de modifier la grille en memoire puis de TOUT redessiner,
        // on parcourt la grille et on ne dessine QUE les mines qui apparaissent
        for (int y = 0; y < b.getH(); y++)
        {
            for (int x = 0; x < b.getW(); x++)
            {
                Cell &c = b.get(x, y);

                // Si c'est une mine non decouverte, ou un faux drapeau
                if ((c.isMine && !c.isFlagged) || (!c.isMine && c.isFlagged))
                {
                    // On met a jour l'etat logique de la case
                    if (c.isMine && !c.isExploded)
                    {
                        c.isRevealed = 1;
                    }
                    else if (!c.isMine && c.isFlagged)
                    {
                        c.isFalseMine = 1;
                    }
                    // On ne redessine QUE cette case precise sur l'ecran VGA !
                    drawSingleCell(x, y);
                }
            }
        }

        v.drawEmoji(emojiX, emojiY, emojiState);
        m.show(); // On reaffiche la souris
    }
    else if (state == STATE_WON)
    {
        emojiState = EMOJI_WON;

        /* Pose automatique des drapeaux sur les mines, puis redessin de ces cases */
        b.flagAllMines();
        for (int y = 0; y < b.getH(); y++)
        {
            for (int x = 0; x < b.getW(); x++)
            {
                if (b.get(x, y).isMine)
                {
                    drawSingleCell(x, y);
                }
            }
        }

        v.drawCounter(mineCounterX, counterY, b.getRemainingMines());
        v.drawEmoji(emojiX, emojiY, emojiState);

        if (elapsedSeconds < 1)
            elapsedSeconds = 1;

        /* Verification d'un nouveau record */
        if (scores.isNewRecord(d, elapsedSeconds))
        {
            promptNewRecord(d, elapsedSeconds);
            showHighScores();
            drawFullInterface();
        }
    }
    m.show();
    pressedOnEmoji = 0;

    /* Boucle finale d'attente avec comportement poussoir de l'emoji */
    while (state != STATE_PLAYING)
    {
        if (kbhit())
        {
            readKey();
            m.hide();
            return 0;
        }

        m.getStatus(mx, my, mb);
        int emojiH2 = (v.getIsVGA() == 1) ? 24 : 18;
        int onEmojiEnd = (mx >= emojiX && mx < emojiX + 24 && my >= emojiY && my < emojiY + emojiH2);

        int targetEmojiEnd = (state == STATE_WON) ? EMOJI_WON : EMOJI_LOST;
        if (onEmojiEnd && (mb & 1) && pressedOnEmoji)
            targetEmojiEnd = EMOJI_CLICKED;

        if (targetEmojiEnd != emojiState)
        {
            emojiState = targetEmojiEnd;
            m.hide();
            v.drawEmoji(emojiX, emojiY, emojiState);
            m.show();
        }

        if ((mb & 1) && !(lastB & 1))
        {
            if (onEmojiEnd)
                pressedOnEmoji = 1;
        }

        if (!(mb & 1) && (lastB & 1))
        {
            if (pressedOnEmoji && onEmojiEnd)
            {
                pressedOnEmoji = 0;
                return 1;
            }
            pressedOnEmoji = 0;
        }
        lastB = mb;
        idle();
    }

    return 0;
}

void MinesApp::showHighScores()
{
redraw_scores:
    int isVga = (v.getIsVGA() == 1);
    int screenH = isVga ? 480 : 350;

    int pW = 320;
    int pH = isVga ? 210 : 154; /* 210 * 0.73 = 154 px */
    int pX = (640 - pW) / 2;
    int pY = (screenH - pH) / 2;

    v.drawPanel(pX, pY, pW, pH, 1);
    v.setTextColor();

    /* Titre des scores adapte (Taille 2 en VGA, Taille 1 en EGA) */
    settextstyle(SANS_SERIF_FONT, HORIZ_DIR, isVga ? 2 : 1);
    const char *hTitle = STR_MENU_SCORES_TITLE;
    int tx = pX + (pW - textW((char *)hTitle)) / 2;
    drawText(tx, pY + (isVga ? 12 : 8), (char *)hTitle);

    settextstyle(DEFAULT_FONT, HORIZ_DIR, 1);
    const char *diffNames[3] = {STR_MENU_SCORES_BEGINNER, STR_MENU_SCORES_INTERMED, STR_MENU_SCORES_ADVANCED};
    char lineBuf[64];

    /* Espacement vertical des lignes reduit en EGA (21px au lieu de 28px) */
    int rowSpacing = isVga ? 28 : 21;
    int tableTop = isVga ? 60 : 44;

    for (int i = 0; i < 3; i++)
    {
        const HighScore &s = scores.get((Difficulty)i);
        sprintf(lineBuf, "%-14s: %3d s  %-10s", diffNames[i], s.time, s.name);
        drawText(pX + 24, pY + tableTop + i * rowSpacing, lineBuf);
    }
    if (scores.hasSaveError())
    {
        drawText(pX + 24, pY + (isVga ? 136 : 100), STR_MENU_SCORES_SAVEERR);
    }

    /* Dimensions et positions des boutons Ok et Reset */
    int btnH = isVga ? 30 : 22;
    int btnResetY = pY + (isVga ? 155 : 115);
    int btnResetX = pX + 20, btnResetW = 130;
    int btnOkX = pX + 175, btnOkW = 120;
    int textPadding = isVga ? 11 : 6;

    v.drawPanel(btnResetX, btnResetY, btnResetW, btnH, 1);
    v.setTextColor();
    drawText(btnResetX + 10, btnResetY + textPadding, STR_MENU_SCORES_RESET);

    v.drawPanel(btnOkX, btnResetY, btnOkW, btnH, 1);
    v.setTextColor();
    drawText(btnOkX + 48, btnResetY + textPadding, STR_MENU_SCORES_OK);

    m.show();
    int mx, my, mb, lastB = 0;
    int pressed = 0;

    while (1)
    {
        if (kbhit())
        {
            int ch = readKey();
            if (ch == 27 || ch == 13 || ch == 'o' || ch == 'O')
                break;
            if (ch == 'r' || ch == 'R')
            {
                scores.reset();
                scores.save();
                m.hide();
                goto redraw_scores;
            }
        }

        m.getStatus(mx, my, mb);

        if ((mb & 1) && !(lastB & 1))
        {
            if (my >= btnResetY && my <= btnResetY + btnH)
            {
                if (mx >= btnResetX && mx <= btnResetX + btnResetW)
                    pressed = 1;
                else if (mx >= btnOkX && mx <= btnOkX + btnOkW)
                    pressed = 2;
            }
        }

        if (!(mb & 1) && (lastB & 1))
        {
            if (pressed == 1 && (mx >= btnResetX && mx <= btnResetX + btnResetW && my >= btnResetY && my <= btnResetY + btnH))
            {
                scores.reset();
                scores.save();
                m.hide();
                goto redraw_scores;
            }
            else if (mx >= btnOkX && mx <= btnOkX + btnOkW && my >= btnResetY && my <= btnResetY + btnH)
            {
                break;
            }
            pressed = 0;
        }
        lastB = mb;
        idle();
    }
    m.hide();
}

void MinesApp::promptNewRecord(Difficulty d, int seconds)
{
    int isVga = (v.getIsVGA() == 1);
    int screenH = isVga ? 480 : 350;

    int pW = 340;
    int pH = isVga ? 190 : 140; /* 190 * 0.73 = 138.7 -> 140 px */
    int pX = (640 - pW) / 2;
    int pY = (screenH - pH) / 2;

    v.drawPanel(pX, pY, pW, pH, 1);
    v.setTextColor();

    /* Sous-titre vectoriel */
    settextstyle(SANS_SERIF_FONT, HORIZ_DIR, isVga ? 2 : 1);
    const char *hTitle = STR_MENU_SCORES_NEWRECORD1;
    int tx = pX + (pW - textW((char *)hTitle)) / 2;
    drawText(tx, pY + (isVga ? 12 : 8), (char *)hTitle);

    settextstyle(DEFAULT_FONT, HORIZ_DIR, 1);
    char buf[64];
    const char *diffNames[3] = {STR_MENU_SCORES_BEGINNER, STR_MENU_SCORES_INTERMED, STR_MENU_SCORES_ADVANCED};
    sprintf(buf, STR_MENU_SCORES_NEWRECORD2, diffNames[(int)d], seconds);
    drawText(pX + (pW - textW(buf)) / 2, pY + (isVga ? 50 : 38), buf);

    drawText(pX + 25, pY + (isVga ? 76 : 58), STR_MENU_SCORES_NAME);

    /* Zone d'edition de texte (Sunken) ecrasee en EGA (18px de haut au lieu de 24px) */
    int editX = pX + 25, editY = pY + (isVga ? 96 : 74), editW = 290;
    int editH = isVga ? 24 : 18;
    v.drawSunkenRect(editX, editY, editW, editH);

    /* Bouton OK de validation */
    int btnOkW = 100;
    int btnOkH = isVga ? 28 : 21;
    int btnOkX = pX + (pW - btnOkW) / 2;
    int btnOkY = pY + (isVga ? 138 : 105);
    int textPaddingOk = isVga ? 10 : 6;

    v.drawPanel(btnOkX, btnOkY, btnOkW, btnOkH, 1);
    v.setTextColor();
    drawText(btnOkX + 40, btnOkY + textPaddingOk, STR_MENU_SCORES_OK);

    char name[16];
    name[0] = '\0';
    int len = 0;

    m.show();
    int mx, my, mb, lastB = 0;
    int pressedOk = 0;

    while (1)
    {
        if (kbhit())
        {
            int ch = readKey();
            if (ch == 13 || ch == 27)
            {
                break;
            }
            else if (ch == 8)
            {
                if (len > 0)
                {
                    len--;
                    name[len] = '\0';
                }
            }
            else if (ch >= 32 && ch <= 126 && len < 15)
            {
                name[len++] = ch;
                name[len] = '\0';
            }

            m.hide();
            v.drawSunkenRect(editX, editY, editW, editH);
            setcolor(WHITE);
            char dispBuf[20];
            sprintf(dispBuf, "%s_", name);
            /* Centrage vertical du texte saisi dans la boite */
            drawText(editX + 8, editY + (isVga ? 8 : 5), dispBuf);
            m.show();
        }

        m.getStatus(mx, my, mb);

        if ((mb & 1) && !(lastB & 1))
        {
            if (mx >= btnOkX && mx <= btnOkX + btnOkW && my >= btnOkY && my <= btnOkY + btnOkH)
            {
                pressedOk = 1;
            }
        }

        if (!(mb & 1) && (lastB & 1))
        {
            if (pressedOk && mx >= btnOkX && mx <= btnOkX + btnOkW && my >= btnOkY && my <= btnOkY + btnOkH)
            {
                break;
            }
            pressedOk = 0;
        }
        lastB = mb;
        idle();
    }

    m.hide();
    if (len > 0)
    {
        scores.update(d, name, seconds);
    }
    else
    {
        scores.update(d, STR_MENU_SCORES_DEFAULT, seconds);
    }
}

void MinesApp::renderDirtyCells()
{
    int count = b.getDirtyCount();
    if (count == 0)
        return;

    const Point *dirty = b.getDirtyCells();

    m.hide();
    for (int i = 0; i < count; i++)
    {
        drawSingleCell(dirty[i].x, dirty[i].y);
    }
    m.show();

    b.clearDirty();
}
