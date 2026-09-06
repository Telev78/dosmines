#include <graphics.h>
#include <dos.h>
#include <stdio.h>
#include <conio.h>
#include "minesapp.h"

MinesApp::MinesApp() : curX(0), curY(0), curB(0), lastB(0), offX(0), offY(0) {
}

void MinesApp::run() {
    if (!m.init()) return;
    menu();
}

void MinesApp::menu() {
    cleardevice();
    v.drawBezel(220, 150, 200, 150, 1);
    outtextxy(260, 170, "DOS MINES");
    outtextxy(240, 210, "1. DEBUTANT");
    outtextxy(240, 240, "2. INTERMEDIAIRE");
    outtextxy(240, 270, "3. AVANCE");

    m.show();
    while (!kbhit()) {
        m.getStatus(curX, curY, curB);
        if (curB & 1) {
            if (curX > 240 && curX < 380) {
                if (curY > 200 && curY < 225) play(BEGINNER);
                else if (curY > 230 && curY < 255) play(INTERMEDIATE);
                else if (curY > 260 && curY < 285) play(ADVANCED);
            }
        }
    }
}

void MinesApp::play(Difficulty d) {
    m.hide();
    b.setup(d);
    offX = (640 - b.getW() * 16) / 2;
    offY = (480 - b.getH() * 16) / 2 + 20;
    cleardevice();
    drawBoard();

    int running = 1;
    m.show();
    while (running && !kbhit()) {
        m.getStatus(curX, curY, curB);
        if (curB != lastB && curB != 0) {
            int gx = (curX - offX) / 16;
            int gy = (curY - offY) / 16;
            if (gx >= 0 && gx < b.getW() && gy >= 0 && gy < b.getH()) {
                m.hide();
                if (curB & 1) {
                    if (b.get(gx, gy).isMine) running = 0;
                    else b.reveal(gx, gy);
                } else if (curB & 2) {
                    b.get(gx, gy).isFlagged = !b.get(gx, gy).isFlagged;
                }
                drawBoard();
                m.show();
            }
        }
        lastB = curB;
    }
    m.hide();
    if (!running) {
        setcolor(RED);
        outtextxy(250, 10, "BOOM! GAME OVER");
        delay(2000);
    }
    menu();
}

void MinesApp::drawBoard() {
    v.drawBezel(offX - 5, offY - 5, b.getW() * 16 + 10, b.getH() * 16 + 10, 0);
    for (int y = 0; y < b.getH(); y++) {
        for (int x = 0; x < b.getW(); x++) {
            Cell &c = b.get(x, y);
            int dx = offX + x * 16;
            int dy = offY + y * 16;
            if (!c.isRevealed) {
                v.drawBezel(dx, dy, 15, 15, 1);
                if (c.isFlagged) {
                    setcolor(RED);
                    outtextxy(dx + 4, dy + 4, "F");
                }
            } else {
                setfillstyle(SOLID_FILL, LIGHTGRAY);
                bar(dx, dy, dx + 15, dy + 15);
                rectangle(dx, dy, dx + 15, dy + 15);
                if (c.count > 0) {
                    char buf[2];
                    sprintf(buf, "%d", c.count);
                    setcolor(c.count);
                    outtextxy(dx + 5, dy + 4, buf);
                }
            }
        }
    }
}
