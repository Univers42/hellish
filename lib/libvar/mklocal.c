#include "libvar.h"

static s_localvar	*handle_special_opt(void)
{
	struct s_var_state	*state;
	struct s_localvar	*lvp;
	char				*p;

	state = get_var_state();
	lvp = ckmalloc(sizeof(struct s_localvar));
	p = ckmalloc(NOPTS);
	lvp->text = memcpy(p, state->optlist, NOPTS);
	lvp->vp = NULL;
	return (lvp);
}

static void	setup_new_local(struct s_localvar *lvp, char *name, int flags)
{
	char	*eq;

	eq = strchr(name, '=');
	if (eq)
		lvp->vp = setvareq(name, VSTRFIXED | flags);
	else
		lvp->vp = setvar(name, NULL, VSTRFIXED | flags);
	lvp->flags = VUNSET;
}

static void	setup_existing_local(struct s_localvar *lvp, struct s_var *vp,
								char *name, int flags)
{
	char	*eq;

	eq = strchr(name, '=');
	lvp->text = vp->text;
	lvp->flags = vp->flags;
	vp->flags |= VSTRFIXED | VTEXTFIXED;
	if (eq)
		setvareq(name, flags);
	lvp->vp = vp;
}

static s_localvar	*handle_var_opt(char *name, int flags)
{
	struct s_localvar	*lvp;
	struct s_var		*vp;

	lvp = ckmalloc(sizeof(struct s_localvar));
	vp = *var_find(var_hash(name), name);
	if (vp == NULL)
		setup_new_local(lvp, name, flags);
	else
		setup_existing_local(lvp, vp, name, flags);
	return (lvp);
}

void	mklocal(char *name, int flags)
{
	struct s_var_state	*state;
	struct s_localvar	*lvp;

	state = get_var_state();
	INTOFF;
	if (name[0] == '-' && name[1] == '\0')
		lvp = handle_special_opt();
	else
		lvp = handle_var_opt(name, flags);
	lvp->next = state->localvar_stack->lv;
	state->localvar_stack->lv = lvp;
	INTON;
}
