#include "includes.h"

int main(int argc, char **argv)
{
    SNMPC snmpc;

    initSNMPC(&snmpc);

    if(snmpc.running)
    {
        initThreads(&snmpc);
        waitThreads(&snmpc);
    }

    freeSNMPC(&snmpc);
    return EXIT_SUCCESS;
}
