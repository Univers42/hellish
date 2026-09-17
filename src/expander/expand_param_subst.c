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
#include "case_match.h"

/* Find how many characters of s[0..] are consumed by a match of pat.
   Returns the match length (may be 0 for a pattern that matches empty), or
   -1 if the pattern does not match at this position.  Literal patterns (no
   wildcards) take an O(m) ft_strncmp fast path; wildcard patterns try
   decreasing lengths until pat_match_pub succeeds (longest-match scan
   needed because * can expand to varying amounts).  Shared with the
   anchored #/% forms in expand_param_subst2.c.

   `[` belongs in the metachar set and was not there: a bracket expression
   took the literal path, so ${v//[X]/-} compared the three characters `[X]`
   against the value and replaced nothing -- silently, because "no match" is
   an ordinary answer here. A fast path has to know every character the
   matcher treats as special, which is the standing cost of having one.
   `(` is the last of them, and it was missing the same way:

     v=aXbYc; echo "${v//@(X|Y)/-}"     bash: a-b-c      here: aXbYc
     v=aaa;   echo "${v//+(a)/Z}"       bash: Z          here: aaa

   -- an extglob group whose operator is not itself a wildcard (`@(`, `+(`,
   `!(`, and zsh's bare `(`) has no character in the old set, so the whole
   group was compared as literal text and matched nothing.  Every group
   spells its opening paren, so testing for that one byte covers all four
   without the fast path having to learn what an operator is. */
int	patsub_match_len(const char *pat, const char *s)
{
	size_t	plen;
	int		k;
	int		cap;

	plen = ft_strlen(pat);
	if (!ft_strchr(pat, '*') && !ft_strchr(pat, '?') && !ft_strchr(pat, '[')
		&& !ft_strchr(pat, '\\') && !ft_strchr(pat, '('))
	{
		if (!ft_strncmp(pat, s, plen))
			return ((int)plen);
		return (-1);
	}
	if (!patsub_head_ok(pat, plen, s))
		return (-1);
	cap = patsub_maxlen(pat, plen);
	k = 0;
	while (s[k] && (cap < 0 || k < cap))
		k++;
	while (k >= 0)
	{
		if (case_match_n(s, (size_t)k, pat, plen))
			return (k);
		k--;
	}
	return (-1);
}

/* Build the substituted string by walking `val` one character at a time.
   When global=0 only the first match is replaced; when global=1 every
   non-overlapping match is replaced.  Unmatched characters and zero-length
   match results are copied literally to avoid infinite loops.  The output
   is grown on demand in `out` via vec_push.

   `live` is patsub_anywhere's answer about the rest of the value: -1 not
   asked yet, 0 nothing left to find, 1 something is.  Asking it turns the
   common "this pattern matches nothing" case from a match at every position
   into a single one, and it doubles as the old `done` gate: a replacement
   sets it back to -1 when every match is wanted and to 0 when only the
   first was, which is what `-global` spells. */
static char	*patsub_build(const char *val, const char *pat,
				const char *rep, int global)
{
	t_string	out;
	int			i;
	int			k;
	int			live;

	vec_init(&out);
	out.elem_size = 1;
	i = 0;
	live = -1;
	while (val[i])
	{
		k = -1;
		if (pat[0] && live < 0)
			live = patsub_anywhere(pat, val + i);
		if (pat[0] && live > 0)
			k = patsub_match_len(pat, val + i);
		if (k > 0)
			(vec_push_str(&out, (char *)rep), i += k, live = -global);
		else
			(vec_push_char(&out, val[i]), i++);
	}
	return (vec_push_char(&out, '\0'), (char *)out.ctx);
}

/* Extract and expand the pattern portion of a ${v/pat/rep} spec: the span
   subst_span finds, after the operator slash(es) and any '#'/'%' anchor,
   up to the first '/' that is not quoted, escaped or nested.  We call
   expand_param_pattern so nested ${} and `...` inside it are processed. */
static char	*subst_get_pat(t_shell *state, t_trim_ctx ctx, int g, int a)
{
	int	start;
	int	end;

	end = subst_span(ctx, g, a, &start);
	return (expand_param_pattern(state, ctx.name + start, end - start));
}

/* Extract and expand the replacement part of ${v/pat/rep}.  If there is no
   separating '/' the form is ${v/pat} which is a pure deletion (empty rep).
   expand_param_word handles nested expansions in the replacement too. */
static char	*subst_get_rep(t_shell *state, t_trim_ctx ctx, int g, int a)
{
	int	start;
	int	end;

	end = subst_span(ctx, g, a, &start);
	if (end < ctx.slen)
		return (expand_param_word(state, ctx.name + end + 1,
				ctx.slen - end - 1, false));
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
	pat = subst_get_pat(state, ctx, g, a);
	rep = subst_get_rep(state, ctx, g, a);
	if (a == 2)
		val = patsub_prefix(val, pat, rep);
	else if (a == 3)
		val = patsub_suffix(val, pat, rep);
	else
		val = patsub_build(val, pat, rep, g);
	return (xfree(pat), xfree(rep), val);
}
