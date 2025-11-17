
#include "libvar.h"

static void	restore_opt_var(struct s_localvar *lvp, struct s_var_state *state)
{
	ft_memcpy(state->optlist, lvp->text, NOPTS);
	ckfree(lvp->text);
	optschanged();
}

static void	restore_regular_var(struct s_localvar *lvp)
{
	struct s_var	*vp;

	vp = lvp->vp;
	if (lvp->flags == VUNSET)
	{
		vp->flags &= ~(VSTRFIXED | VREADONLY);
		unsetvar(vp->text);
	}
	else
	{
		if (vp->func)
			(*vp->func)(strchrnul(lvp->text, '=') + 1);
		if ((vp->flags & (VTEXTFIXED | VSTACK)) == 0)
			ckfree((void *)vp->text);
		vp->flags = lvp->flags;
		vp->text = lvp->text;
	}
}

static void	process_local_var_list(struct s_localvar *lvp_list,
								struct s_var_state *state)
{
	struct s_localvar	*current;
	struct s_localvar	*next;

	current = lvp_list;
	while (current != NULL)
	{
		next = current->next;
		if (current->vp == NULL)
			restore_opt_var(current, state);
		else
			restore_regular_var(current);
		ckfree(current);
		current = next;
	}
}

void	poplocalvars(void)
{
	t_localvar_list	*ll;
	t_var_state		*state;
	t_localvar		*lvp_list;

	state = get_var_state();
	INTOFF;
	ll = state->localvar_stack;
	state->localvar_stack = ll->next;
	lvp_list = ll->lv;
	ckfree(ll);
	process_local_var_list(lvp_list, state);
	INTON;
}
