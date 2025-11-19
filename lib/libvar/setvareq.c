#include "libvar.h"

// External dependencies
#include <stddef.h>
#ifndef INTOFF
#define INTOFF /* nothing or your shell's macro */
#endif
#ifndef INTON
#define INTON /* nothing or your shell's macro */
#endif
extern void ckfree(void *ptr);
extern void *ckmalloc(size_t size);
extern char *savestr(const char *s);
extern void sh_error(const char *, ...);
extern char *strchrnul(const char *s, int c);

static struct s_var *create_new_var(struct s_var **vpp, char *s, int flags)
{
	struct s_var *vp;

	if (flags & VNOSET)
		return (NULL);
	if ((flags & (VEXPORT | VREADONLY | VSTRFIXED | VUNSET)) == VUNSET)
	{
		if ((flags & (VTEXTFIXED | VSTACK | VNOSAVE)) == VNOSAVE)
			ckfree(s);
		return (NULL);
	}
	vp = ckmalloc(sizeof(*vp));
	vp->next = *vpp;
	vp->func = NULL;
	*vpp = vp;
	if (!(flags & (VTEXTFIXED | VSTACK | VNOSAVE)))
		s = savestr(s);
	vp->text = s;
	vp->flags = flags;
	return (vp);
}

static int handle_update_unset(struct s_var **vpp, char *s, int flags)
{
	struct s_var *vp;
	int total_flags;

	vp = *vpp;
	total_flags = (flags & (VEXPORT | VREADONLY | VSTRFIXED | VUNSET)) |
				  (vp->flags & VSTRFIXED);
	if (total_flags == VUNSET)
	{
		*vpp = vp->next;
		ckfree(vp);
		if ((flags & (VTEXTFIXED | VSTACK | VNOSAVE)) == VNOSAVE)
			ckfree(s);
		return (1);
	}
	return (0);
}

static int check_var_permissions(struct s_var *vp, char *s, int flags)
{
	const char *n;

	if (vp->flags & VREADONLY)
	{
		if (flags & VNOSAVE)
			ckfree(s);
		n = vp->text;
		sh_error("%.*s: is read only", (int)(strchrnul(n, '=') - n), n);
		return (1);
	}
	if (flags & VNOSET)
		return (1);
	return (0);
}

static struct s_var *update_existing_var(struct s_var **vpp, char *s, int flags)
{
	struct s_var *vp;

	vp = *vpp;
	if (check_var_permissions(vp, s, flags))
		return (vp);
	if (vp->func && (flags & VNOFUNC) == 0)
		(*vp->func)(strchrnul(s, '=') + 1);
	if ((vp->flags & (VTEXTFIXED | VSTACK)) == 0)
		ckfree((void *)vp->text);
	if (handle_update_unset(vpp, s, flags))
		return (NULL);
	flags |= vp->flags & ~(VTEXTFIXED | VSTACK | VNOSAVE | VUNSET);
	if (!(flags & (VTEXTFIXED | VSTACK | VNOSAVE)))
		s = savestr(s);
	vp->text = s;
	vp->flags = flags;
	return (vp);
}

struct s_var *setvareq(char *s, int flags)
{
	struct s_var *vp;
	struct s_var **vpp;
	extern int aflag;

	flags |= (VEXPORT & (((unsigned)(1 - aflag)) - 1));
	vpp = var_hash(s);
	vpp = var_find(vpp, s);
	vp = *vpp;
	INTOFF;
	if (vp)
		vp = update_existing_var(vpp, s, flags);
	else
		vp = create_new_var(vpp, s, flags);
	INTON;
	return (vp);
}
