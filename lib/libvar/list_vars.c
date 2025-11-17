#include "libvar.h"

static char	**process_var_bucket(struct s_var *vp, char **ep, int mask, int on)
{
	while (vp)
	{
		if ((vp->flags & mask) == on)
		{
			if (ep == stackstrend())
				ep = growstackstr();
			*ep++ = (char *)vp->text;
		}
		vp = vp->next;
	}
	return (ep);
}

char	**listvars(int on, int off, char ***end)
{
	struct s_var_state	*state;
	struct s_var		**vpp;
	char				**ep;
	int					mask;

	state = get_var_state();
	STARTSTACKSTR(ep);
	vpp = state->vartab;
	mask = on | off;
	while (vpp < state->vartab + VTABSIZE)
	{
		ep = process_var_bucket(*vpp, ep, mask, on);
		vpp++;
	}
	if (ep == stackstrend())
		ep = growstackstr();
	if (end)
		*end = ep;
	*ep++ = NULL;
	return (grabstackstr(ep));
}
