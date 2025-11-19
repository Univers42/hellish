
#include "libvar.h"

struct s_var	**var_find(struct s_var **vpp, const char *name)
{
	while (*vpp)
	{
		if (libvar_varcmp((*vpp)->text, name) == 0)
			break ;
		vpp = &(*vpp)->next;
	}
	return (vpp);
}

