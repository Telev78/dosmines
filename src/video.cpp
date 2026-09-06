#include <graphics.h>
#include <stdlib.h>
#include <stdio.h>
#include <alloc.h>
#include "video.h"

Video::Video() {
    registerbgidriver(EGAVGA_driver);
    int gdir = VGA, gmod = VGAHI;
    initgraph(&gdir, &gmod, "");
}

Video::~Video() {
    closegraph();
}

void* Video::loadFromBMP(FILE* f, int x, int y, int w, int h) {
    (void)f;
    (void)x;
    (void)y;
    void* ptr;
    unsigned int sz = imagesize(0, 0, w - 1, h - 1);
    ptr = malloc(sz);
    getimage(0, 0, w - 1, h - 1, ptr);
    return ptr;
}

void Video::drawBezel(int x, int y, int w, int h, int out) {
    setcolor(out ? WHITE : DARKGRAY);
    line(x, y, x + w, y);
    line(x, y, x, y + h);
    setcolor(out ? DARKGRAY : WHITE);
    line(x + w, y, x + w, y + h);
    line(x, y + h, x + w, y + h);
}
