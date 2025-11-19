#include "libvar.h"

extern void sh_error(const char *, ...); // Add this line

int localcmd(int argc, char **argv)
{
	struct s_var_state *state;
	int i;

	state = get_var_state();
	if (!state->localvar_stack)
		sh_error("not in a function");
	i = 1;
	while (i < argc)
	{
		mklocal(argv[i], 0);
		i++;
	}
	return (0);
}
