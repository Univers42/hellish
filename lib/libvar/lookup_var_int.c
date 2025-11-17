#include "libvar.h"

intmax_t	lookupvarint(const char *name)
{
	char	*value;

	value = lookupvar(name);
	if (value == NULL)
		return (ft_atomax("", 0));
	return (ft_atomax(value, 0));
}
