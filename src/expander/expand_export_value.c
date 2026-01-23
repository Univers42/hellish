/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   expand_export_value.c                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/23 14:04:51 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/23 14:42:44 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../builtins/builtins_private.h"

// Helper: Append a variable expansion to the output buffer
static void	append_var_expansion(t_shell *st, t_string *out, char *val, size_t start, size_t end)
{
	char	*rep;

	rep = env_expand_n(st, val + start, (int)(end - start));
	if (rep)
		vec_push_str(out, rep);
}

// Helper: Expand a single character to the output buffer
static void	append_char(t_string *out, char c)
{
	vec_push(out, &c);
}

// Helper: Expand variables in a string and append to output buffer
static void	expand_vars_to_buffer(t_shell *st, t_string *out, char *val)
{
	size_t	i;
	size_t	start;

	i = 0;
	while (val[i])
	{
		if (val[i] == '$' && is_var_name_p1(val[i + 1]))
		{
			start = ++i;
			while (is_var_name_p2(val[i]))
				i++;
			append_var_expansion(st, out, val, start, i);
			continue ;
		}
		append_char(out, val[i]);
		i++;
	}
}

char	*expand_export_value(t_shell *st, char *val, bool allow_expand)
{
	t_string	out;

	if (!val || !allow_expand)
		return (val);
	vec_init(&out);
	out.elem_size = 1;
	expand_vars_to_buffer(st, &out, val);
	free(val);
	if (!vec_ensure_space_n(&out, 1))
		return (NULL);
	((char *)out.ctx)[out.len] = '\0';
	return ((char *)out.ctx);
}
