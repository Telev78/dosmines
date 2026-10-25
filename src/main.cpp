#include "minesapp.h"
#include <dos.h>

extern unsigned _stklen = 16384U; /* 16 Ko au lieu de 4 Ko */

int main(int argc, char *argv[])
{
    (void)argc;

    if (_osmajor < 3)
    {
        printf("\n%s\n", STR_ERR_DOS);
        return 1;
    }

    MinesApp *app = new MinesApp();
    if (app)
    {
        app->run(argv[0]);
        delete app;
    }
    else
    {
        printf("\n%s\n", STR_ERR_MEMORY);
    }

    return 0;
}
