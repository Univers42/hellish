#include <inttypes.h>
#include <stdlib.h>

intmax_t ft_atomax(const char *s, int base)
{
    char *endptr;
    return strtoimax(s, &endptr, base);
}
