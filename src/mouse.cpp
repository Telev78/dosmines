#include <dos.h>
#include "mouse.h"

#define MOUSE_INT 0x33

int Mouse::init() {
    union REGS r;
    r.x.ax = 0;
    int86(MOUSE_INT, &r, &r);
    return r.x.ax;
}

void Mouse::show() {
    union REGS r;
    r.x.ax = 1;
    int86(MOUSE_INT, &r, &r);
}

void Mouse::hide() {
    union REGS r;
    r.x.ax = 2;
    int86(MOUSE_INT, &r, &r);
}

void Mouse::getStatus(int &x, int &y, int &b) {
    union REGS r;
    r.x.ax = 3;
    int86(MOUSE_INT, &r, &r);
    x = r.x.cx;
    y = r.x.dx;
    b = r.x.bx;
}
