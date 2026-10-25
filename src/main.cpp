#include "minesapp.h"

extern unsigned _stklen = 16384U; /* 16 Ko au lieu de 4 Ko */

int main(int argc, char *argv[])
{
    (void)argc;

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
