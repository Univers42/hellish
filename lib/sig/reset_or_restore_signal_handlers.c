#include "trap.h"

void reset_or_restore_signal_handlers(void (*func)(int))
{
    int i;

    if (!func)
        return;
    i = 1;
    while (i < NSIG)
    {
        func(i);
        i++;
    }
}
