#include "trap.h"

void initialize_signames(void)
{
    int i;

    i = 0;
    while (i < NSIG)
    {
        get_g_sig()->signal_name[i] = (char *)strsignal(i);
        i++;
    }
    /* Custom (DEBUG/ERROR/RETURN/EXIT) are not stored here since signal_name
       is only NSIG entries; signal_name() handles those explicitly. */
}
