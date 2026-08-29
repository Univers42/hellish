/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   expand_zsh_disp2.c                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/30 09:35:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/30 09:35:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"
#include "env.h"
#include "ft_builtins.h"

/* Helpers for the nested-expansion path (expand_zsh_disp.c). */

/* Expand one `${...}` given its full text INCLUDING the braces, and hand
   back an owned string.  Routes through expand_param_format so the inner
   expansion gets everything -- flags, filters, modifiers, operators, and
   further nesting -- rather than a second, poorer reader. */
char	*zf_inner_text(t_shell *state, const char *s, int n)
{
	char	*out;

	if (n < 3)
		return (ft_strdup(""));
	out = zsh_token_text(state, s + 2, n - 3);
	if (out)
		return (out);
	out = expand_param_format(state, s + 2, n - 3, false);
	if (out)
		return (out);
	return (zd_plain(state, s + 2, n - 3));
}

/* A plain name lookup that always returns an owned string, never NULL: the
   empty string is what an unset parameter contributes, and NULL there would
   be indistinguishable from "this form was not handled".
     Takes a LENGTH, not a C string. The name is a slice of the enclosing
   body -- `A` inside `${${A}:-b}` is followed by `}:-b`, not by a NUL -- and
   reading it as a C string looked up a variable called "A}:-b". */
char	*zd_plain(t_shell *state, const char *name, int len)
{
	char	*v;

	v = env_expand_n(state, (char *)name, len);
	if (!v)
		return (ft_strdup(""));
	return (ft_strdup(v));
}

/* The scratch parameter one nesting level binds its inner value to.
** `depth` keeps levels apart, so a doubly-nested body cannot clobber the
** level above it while that level is still being read. */
char	*zd_bind_name(int depth)
{
	char	*n;
	char	*out;

	n = ft_itoa(depth);
	if (!n)
		return (NULL);
	out = ft_strjoin("__hsh_nest_", n);
	xfree(n);
	return (out);
}

/* Drop the scratch parameter once the outer operator has read it.
   try_unset is the `unset` builtin's own path, so the entry leaves the
   environment exactly the way any other unset would -- no second removal
   routine that could disagree with it about indices or attributes. */
void	zd_unbind(t_shell *state, const char *name)
{
	try_unset(state, (char *)name);
}

/* Build `<name><rest>`: the scratch parameter followed by whatever operator
   the outer braces carried. */
char	*zd_splice(const char *val, const char *rest, int rlen)
{
	t_string	out;
	int			i;

	vec_init(&out);
	out.elem_size = 1;
	vec_push_str(&out, (char *)val);
	i = -1;
	while (++i < rlen)
		vec_push_char(&out, rest[i]);
	vec_push_char(&out, '\0');
	return ((char *)out.ctx);
}
