/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   expand_param_format3.c                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/03/14 09:30:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/03/14 09:30:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"

char	*trim_suffix_longest(const char *val, const char *pattern)
{
	int	i;

	i = 0;
	while (val[i])
	{
		if (pat_match_pub(pattern, val + i))
			return (ft_strndup(val, i));
		i++;
	}
	return (ft_strdup(val));
}

char	*trim_prefix_shortest(const char *val, const char *pattern)
{
	int		vlen;
	int		i;
	char	*sub;

	vlen = ft_strlen(val);
	i = 0;
	while (i <= vlen)
	{
		sub = ft_strndup(val, i);
		if (pat_match_pub(pattern, sub))
		{
			free(sub);
			return (ft_strdup(val + i));
		}
		free(sub);
		i++;
	}
	return (ft_strdup(val));
}

char	*trim_prefix_longest(const char *val, const char *pattern)
{
	int		vlen;
	int		i;
	char	*sub;

	vlen = ft_strlen(val);
	i = vlen;
	while (i >= 0)
	{
		sub = ft_strndup(val, i);
		if (pat_match_pub(pattern, sub))
		{
			free(sub);
			return (ft_strdup(val + i));
		}
		free(sub);
		i--;
	}
	return (ft_strdup(val));
}

char	*expand_trim(t_shell *state, t_trim_ctx ctx)
{
	char	*val;
	char	*pat;
	char	*result;
	int		op_off;

	val = pf_get_var_value(state, ctx.name, ctx.name_len);
	if (!val)
		return (ft_strdup(""));
	op_off = 1 + (ctx.op[1] == '%' || ctx.op[1] == '#');
	pat = expand_param_word(state, ctx.op + op_off,
			ctx.slen - ctx.name_len - op_off);
	if (ctx.op[0] == '%' && ctx.op[1] == '%')
		result = trim_suffix_longest(val, pat);
	else if (ctx.op[0] == '%')
		result = trim_suffix_shortest(val, pat);
	else if (ctx.op[0] == '#' && ctx.op[1] == '#')
		result = trim_prefix_longest(val, pat);
	else if (ctx.op[0] == '#')
		result = trim_prefix_shortest(val, pat);
	else
		result = ft_strdup(val);
	return (free(pat), result);
}
