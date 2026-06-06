/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   expand_export_value.c                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/23 14:04:51 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/24 19:58:11 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../builtins/builtins_private.h"
#include "sys.h"

/* Look up substart[0..len) in the environment and append the value to out.
   A missing variable contributes nothing (no error — this is export-time,
   not a regular expansion context). */
static void	append_var_expansion(t_shell *st, t_string *out,
								char *substart, int len)
{
	char	*rep;

	rep = env_expand_n(st, substart, len);
	if (rep)
		vec_push_str(out, rep);
}

/* Append a single literal character to the output buffer. */
static void	append_ch(t_string *out, char c)
{
	vec_push(out, &c);
}

/* Expand $var references inside an export value string.  Only the simple
   $name form (is_var_name_p1 / is_var_name_p2) is expanded — no ${},
   $(()), or `...`.  This is the "lightweight" expansion used by `export
   VAR=$OTHER` where the value comes from a previous env assignment. */
static void	expand_vars_to_buffer(t_shell *st, t_string *out, char *val)
{
	size_t	i;
	size_t	start;

	i = 0;
	while (val[i])
	{
		if (val[i] == IS_DOLLAR && is_var_name_p1(val[i + 1]))
		{
			start = ++i;
			while (is_var_name_p2(val[i]))
				i++;
			append_var_expansion(st, out, val + start,
				(int)(i - start));
			continue ;
		}
		append_ch(out, val[i]);
		i++;
	}
}

/* Expand $var references inside an export string (val), return the result.
   `val` is freed by this function; the caller must NOT use it afterwards.
   When allow_expand is false the original string is returned unchanged.
   This guards against double-expansion of values that are already literals. */
char	*expand_export_value(t_shell *st, char *val, bool allow_expand)
{
	t_string	out;

	if (!val || !allow_expand)
		return (val);
	vec_init(&out);
	out.elem_size = 1;
	expand_vars_to_buffer(st, &out, val);
	xfree(val);
	if (!vec_ensure_space_n(&out, 1))
		return (NULL);
	((char *)out.ctx)[out.len] = '\0';
	return ((char *)out.ctx);
}
