/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   expand_zsh_list.c                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/29 18:10:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/29 18:10:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"

/* Every flagged expansion is a LIST internally, even when it started and
   ends as one string.  Doing it that way is what lets (f), (o), (u) and (j)
   compose in any order -- `${(ou)${(f)x}}` is ordinary zsh -- instead of
   each combination needing its own case.  A vec of owned char *, freed by
   zf_emit once the fields have been handed on. */

void	zl_push(t_vec *l, char *owned)
{
	vec_push(l, &owned);
}

void	zl_free(t_vec *l)
{
	size_t	i;

	i = 0;
	while (i < l->len)
		xfree(((char **)l->ctx)[i++]);
	xfree(l->ctx);
	l->ctx = NULL;
	l->len = 0;
	l->cap = 0;
}

/* Split `v` on the literal string `sep`, keeping empty fields -- which is
   the difference between this and IFS splitting, and the whole reason (f)
   exists.  `x=$'a\n\nb'` is three fields to zsh and two to a shell that
   collapses runs, and a plugin counting lines gets a different answer. */
static void	zl_split(t_vec *l, const char *v, const char *sep)
{
	const char	*p;
	size_t		n;

	n = ft_strlen(sep);
	if (!n)
	{
		while (*v)
			zl_push(l, ft_strndup(v++, 1));
		return ;
	}
	p = ft_strnstr(v, sep, ft_strlen(v));
	while (p)
	{
		zl_push(l, ft_strndup(v, (size_t)(p - v)));
		v = p + n;
		p = ft_strnstr(v, sep, ft_strlen(v));
	}
	zl_push(l, ft_strdup(v));
}

/* Seed the list from the expanded value.  An array or associative value
   arrives still encoded (zf_inner hands back what the environment holds),
   so its own elements are the natural fields and (k)/(v) can pick a side;
   anything else starts as a single field and only the split flags break it
   up.  (z) splits on whitespace the way the shell splits a command line
   would -- runs collapse there, unlike (f). */
t_vec	zl_from(t_shell *state, t_zflags *f, const char *val)
{
	t_vec	l;

	vec_init(&l);
	l.elem_size = sizeof(char *);
	if (arr_is(val) || assoc_is(val))
		return (zl_records(state, f, &l, val), l);
	if (zf_has(f, 'f'))
		zl_split(&l, val, "\n");
	else if (zf_has(f, 's') && f->sep)
		zl_split(&l, val, f->sep);
	else if (zf_has(f, 'z'))
		zl_split_ws(&l, val);
	else
		zl_push(&l, ft_strdup(val));
	return (l);
}
