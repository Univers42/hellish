#include "libvar.h"

int	var_vpcmp(const void *a, const void *b)
{
	return (libvar_varcmp(*(const char **)a, *(const char **)b));
}
