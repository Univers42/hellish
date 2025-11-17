#include "libvar.h"

void	unwindlocalvars(struct localvar_list *stop)
{
	t_var_state	*state;

	state = get_var_state();
	while (state->localvar_stack != stop)
		poplocalvars();
}
