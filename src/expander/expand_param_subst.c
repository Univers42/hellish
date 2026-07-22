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

/* Find how many characters of s[0..] are consumed by a match of pat.
   Returns the match length (may be 0 for a pattern that matches empty), or
   -1 if the pattern does not match at this position.  Literal patterns
   (no * or ? wildcards) take an O(m) ft_strncmp fast path; wildcard patterns
   try decreasing lengths until pat_match_pub succeeds (longest-match scan
   needed because * can expand to varying amounts).  Shared with the
   anchored #/% forms in expand_param_subst2.c. */
int	patsub_match_len(const char *pat, const char *s)
{
	int		k;
	char	*sub;

	if (!ft_strchr(pat, '*') && !ft_strchr(pat, '?') && !ft_strchr(pat, '\\'))
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

/* Build the substituted string by walking `val` one character at a time.
   When global=0 only the first match is replaced (`done` gate); when
   global=1 every non-overlapping match is replaced.  Unmatched characters
   and zero-length match results are copied literally to avoid infinite loops.
   The output is grown on demand in `out` via vec_push. */
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

/* Extract and expand the pattern portion of a ${v/pat/rep} spec.  The
   pattern starts after the first '/' plus `g` extra prefix characters (a
   second '/' for global mode and/or a '#'/'%' anchor) and extends to the
   next '/' or end of spec.  We call expand_param_word so nested ${} and
   `...` inside the pattern are processed. */
static char	*subst_get_pat(t_shell *state, t_trim_ctx ctx, int g)
{
	int	start;
	int	i;

	start = ctx.name_len + 1 + g;
	i = start;
	while (i < ctx.slen && ctx.name[i] != '/')
		i++;
	return (expand_param_pattern(state, ctx.name + start, i - start));
}

/* Extract and expand the replacement part of ${v/pat/rep}.  If there is no
   second '/' the form is ${v/pat} which is a pure deletion (empty rep).
   expand_param_word handles nested expansions in the replacement too. */
static char	*subst_get_rep(t_shell *state, t_trim_ctx ctx, int g)
{
	int	i;

	i = ctx.name_len + 1 + g;
	while (i < ctx.slen && ctx.name[i] != '/')
		i++;
	if (i < ctx.slen)
		return (expand_param_word(state, ctx.name + i + 1,
				ctx.slen - i - 1, false));
	return (ft_strdup(""));
}

/* ${name/pat/rep} (first match), ${name//pat/rep} (every match),
   ${name/#pat/rep} (anchored at the start) and ${name/%pat/rep} (anchored
   at the end); a missing /rep deletes the match. val is borrowed from the
   env, so it is not freed. */
char	*expand_subst(t_shell *state, t_trim_ctx ctx)
{
	char	*val;
	char	*pat;
	char	*rep;
	int		g;
	int		a;

	val = pf_get_var_value(state, ctx.name, ctx.name_len);
	if (!val)
		return (ft_strdup(""));
	g = (ctx.name_len + 1 < ctx.slen && ctx.name[ctx.name_len + 1] == '/');
	a = subst_anchor(ctx, g);
	pat = subst_get_pat(state, ctx, g + (a != 0));
	rep = subst_get_rep(state, ctx, g + (a != 0));
	if (a == 2)
		val = patsub_prefix(val, pat, rep);
	else if (a == 3)
		val = patsub_suffix(val, pat, rep);
	else
		val = patsub_build(val, pat, rep, g);
	return (xfree(pat), xfree(rep), val);
}
