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

/* ${val%%pattern} — remove the LONGEST suffix of `val` that matches
   `pattern`.  Scan from i=0 forward: the first position where the remainder
   of val matches the pattern gives the longest possible suffix to remove. */
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

/* ${val#pattern} — remove the SHORTEST prefix of `val` that matches.
   We try prefixes of increasing length (0, 1, 2, …) and return the first
   match.  This naturally gives the shortest (leftmost) match. */
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
			xfree(sub);
			return (ft_strdup(val + i));
		}
		xfree(sub);
		i++;
	}
	return (ft_strdup(val));
}

/* ${val##pattern} — remove the LONGEST prefix of `val` that matches.
   We try prefixes in decreasing length order (longest first) and return the
   first match, so the suffix after the match is as short as possible. */
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
			xfree(sub);
			return (ft_strdup(val + i));
		}
		xfree(sub);
		i--;
	}
	return (ft_strdup(val));
}

/* Dispatcher for prefix/suffix trimming operators (#, ##, %, %%).
   op_off accounts for single vs double operator (# vs ##, % vs %%):
   1 for single, 2 for double.  The pattern is extracted from the raw spec
   and expanded (so ${x#${y}} works) before being passed to the trimmer. */
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
			ctx.slen - ctx.name_len - op_off, false);
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
	return (xfree(pat), result);
}
