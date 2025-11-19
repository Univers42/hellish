#include "libvar.h"

void	unsetvar(const char *s)
{
	setvar(s, 0, 0);
}
