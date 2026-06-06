/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   expand_param_subst.c                               :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/06 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/06 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"

/* Longest prefix of s the whole pattern matches; -1 if none. Literal patterns
   take a direct O(m) compare; only real globs pay the longest-match scan. */
static int	patsub_match_len(const char *pat, const char *s)
{
	int		k;
	char	*sub;

	if (!ft_strchr(pat, '*') && !ft_strchr(pat, '?'))
	{
		k = (int)ft_strlen(pat);
		if (!ft_strncmp(pat, s, (size_t)k))
			return (k);
		return (-1);
	}
	k = (int)ft_strlen(s);
	while (k >= 0)
	{
		sub = ft_strndup(s, (size_t)k);
		if (sub && pat_match_pub(pat, sub))
			return (xfree(sub), k);
		xfree(sub);
		k--;
	}
	return (-1);
}

/* Walk val, replacing matches of pat by rep (all if global, else the first). */
static char	*patsub_build(const char *val, const char *pat,
				const char *rep, int global)
{
	t_string	out;
	int			i;
	int			k;
	int			done;

	vec_init(&out);
	out.elem_size = 1;
	i = 0;
	done = 0;
	while (val[i])
	{
		k = -1;
		if (pat[0] && (global || !done))
			k = patsub_match_len(pat, val + i);
		if (k > 0)
			(vec_push_str(&out, (char *)rep), i += k, done = 1);
		else
			(vec_push_char(&out, val[i]), i++);
	}
	return (vec_push_char(&out, '\0'), (char *)out.ctx);
}

/* Pattern is from after the '/'(s) up to the next '/' (or end of the spec). */
static char	*subst_get_pat(t_shell *state, t_trim_ctx ctx, int g)
{
	int	start;
	int	i;

	start = ctx.name_len + 1 + g;
	i = start;
	while (i < ctx.slen && ctx.name[i] != '/')
		i++;
	return (expand_param_word(state, ctx.name + start, i - start));
}

/* Replacement is everything after the pattern's terminating '/'; empty if the
   form is ${var/pat} (a deletion). */
static char	*subst_get_rep(t_shell *state, t_trim_ctx ctx, int g)
{
	int	i;

	i = ctx.name_len + 1 + g;
	while (i < ctx.slen && ctx.name[i] != '/')
		i++;
	if (i < ctx.slen)
		return (expand_param_word(state, ctx.name + i + 1, ctx.slen - i - 1));
	return (ft_strdup(""));
}

/* ${name/pat/rep} (first match) and ${name//pat/rep} (every match); a missing
   /rep deletes the match. val is borrowed from the env, so it is not freed. */
char	*expand_subst(t_shell *state, t_trim_ctx ctx)
{
	char	*val;
	char	*pat;
	char	*rep;
	int		g;

	val = pf_get_var_value(state, ctx.name, ctx.name_len);
	if (!val)
		return (ft_strdup(""));
	g = (ctx.name_len + 1 < ctx.slen && ctx.name[ctx.name_len + 1] == '/');
	pat = subst_get_pat(state, ctx, g);
	rep = subst_get_rep(state, ctx, g);
	val = patsub_build(val, pat, rep, g);
	return (xfree(pat), xfree(rep), val);
}
