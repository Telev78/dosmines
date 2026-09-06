#ifndef MOUSE_H
#define MOUSE_H

class Mouse {
public:
    int  init();
    void show();
    void hide();
    void getStatus(int &x, int &y, int &b);
};

#endif
