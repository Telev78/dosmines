#ifndef VIDEO_H
#define VIDEO_H

#include <stdio.h>

class Video {
    void* sprites[40];
public:
    Video();
    ~Video();

    void* loadFromBMP(FILE* f, int x, int y, int w, int h);
    void  drawBezel(int x, int y, int w, int h, int out);
};

#endif
