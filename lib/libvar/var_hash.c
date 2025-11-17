#include "libvar.h"

struct s_var	**var_hash(const char *p)
{
	unsigned int	hashval;
	t_var_state		*state;

	state = get_var_state();
	hashval = ((unsigned char)*p) << 4;
	while (*p && *p != '=')
	{
		hashval += (unsigned char)*p;
		p++;
	}
	return (&state->vartab[hashval % VTABSIZE]);
}
