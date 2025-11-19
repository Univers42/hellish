#include "libvar.h"

#define INTMAX_BUFSIZE 25

extern void ft_fmtstr(char *buf, int size, const char *fmt, ...);

intmax_t setvarint(const char *name, intmax_t val, int flags)
{
	char buf[INTMAX_BUFSIZE];
	ft_fmtstr(buf, INTMAX_BUFSIZE, "%", PRIdMAX, val);
	setvar(name, buf, flags);
	return (val);
}
