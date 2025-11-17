#include "libvar.h"

#define INTMAX_BUFSIZE 25

intmax_t	setvarint(const char *name, intmax_t val, int flags)
{
	ft_fmtstr(buf, INTMAX_BUFSIZE, "%", PRIdMAX, val);
	setvar(name, buf, flags);
	return (val);
}
