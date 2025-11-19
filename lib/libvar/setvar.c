#include "libvar.h"

// External dependencies
#include <stddef.h>
#ifndef INTOFF
#define INTOFF /* nothing or your shell's macro */
#endif
#ifndef INTON
#define INTON /* nothing or your shell's macro */
#endif
extern char *end_of_name(const char *s);
extern char *ft_strchrnul(const char *s, int c);
extern size_t ft_strlen(const char *s);
extern void *ckmalloc(size_t size);
extern void *ft_memcpy(void *dst, const void *src, size_t n);

// Fix prototype: add missing comma and use correct parameter names/types
static t_var *build_and_set(const char *name, const char *val, size_t name_len,
							size_t val_len, int flags)
{
	char *name_eq;
	char *p;
	t_var *vp;

	name_eq = ckmalloc(name_len + val_len + 2); // +2 for '=' and '\0'
	p = ft_memcpy(name_eq, name, name_len);
	if (val)
	{
		*p++ = '=';
		p = ft_memcpy(p, val, val_len);
	}
	*p = '\0';
	vp = setvareq(name_eq, flags | VNOSAVE);
	return (vp);
}

/**
 * @brief Sets or unsets a variable.
 * @param name The name of the variable to set.
 * @param val The value of the variable to set. If NULL, the variable is unset.
 * @param flags Flags to be OR'ed with the variable's flags (e.g., VEXPORT).
 * @return A pointer to the variable structure.
 */
t_var *setvar(const char *name, const char *val, int flags)
{
	size_t name_len;
	size_t val_len;
	char *p;
	char *q;
	t_var *vp;

	q = end_of_name(name);
	p = ft_strchrnul(q, '=');
	name_len = p - name;
	if (!name_len || p != q)
		return (NULL);
	val_len = 0;
	if (!val)
		flags |= VUNSET;
	else
		val_len = ft_strlen(val);
	INTOFF;
	vp = build_and_set(name, val, name_len, val_len, flags);
	INTON;
	return (vp);
}
