#include <dos.h>
#include "mouse.h"

#define MOUSE_INT 0x33

int Mouse::init()
{
    union REGS r;

    /* Pas de pilote si le vecteur est nul ou pointe directement sur un IRET */
    void far *vec = (void far *)getvect(MOUSE_INT);
    if (FP_SEG(vec) == 0 && FP_OFF(vec) == 0)
        return 0;
    if (*(unsigned char far *)vec == 0xCF) /* 0xCF = IRET */
        return 0;

    r.x.ax = 0;
    int86(MOUSE_INT, &r, &r);
    return r.x.ax;
}

void Mouse::show()
{
    union REGS r;
    r.x.ax = 1;
    int86(MOUSE_INT, &r, &r);
}

void Mouse::hide()
{
    union REGS r;
    r.x.ax = 2;
    int86(MOUSE_INT, &r, &r);
}

void Mouse::getStatus(int &x, int &y, int &b)
{
    union REGS r;
    r.x.ax = 3;
    int86(MOUSE_INT, &r, &r);
    x = r.x.cx;
    y = r.x.dx;
    b = r.x.bx;
}
