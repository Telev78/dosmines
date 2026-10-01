#include "minesapp.h"

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    MinesApp *app = new MinesApp();
    if (app)
    {
        app->run();
        delete app;
    }

    return 0;
}
