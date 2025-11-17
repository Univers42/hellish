#include "libvar.h"

int	libvar_varcmp(const char *p, const char *q)
{
	int	c;
	int	d;

	while (1)
	{
		c = *p;
		d = *q;
		if (c != d)
			break ;
		if (!c || c == '=')
			break ;
		p++;
		q++;
	}
	if (c == '=')
		c = 0;
	if (d == '=')
		d = 0;
	return (c - d);
}
