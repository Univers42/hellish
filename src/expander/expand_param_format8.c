/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   expand_param_format8.c                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/01 23:10:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/09/01 23:10:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"

/* Indirection targets bash accepts: a plain parameter name/positional, or
   an array element reference NAME[anything] — `ref="$2[$j]"` is how
   bash-completion addresses its word arrays. Anything else is bash's
   "invalid indirect expansion" class. Also the recursion guard: neither
   accepted shape can begin another '!' round. */
static bool	pf_target_ok(const char *mid)
{
	int			n;
	const char	*br;

	n = (int)ft_strlen(mid);
	if (pf_valid_plain(mid, n))
		return (true);
	br = ft_strchr(mid, '[');
	if (!br || br == mid || mid[n - 1] != ']')
		return (false);
	return (pf_valid_plain(mid, (int)(br - mid)));
}

/* Re-enter the full expander on "${<mid><rest>}": subscripted targets
   (a[i], assoc[k]) resolve through the same element machinery the direct
   spelling uses, so indirection cannot drift from it. */
char	*pf_expand_rebuilt(t_shell *state, const char *mid,
			const char *rest, int rlen)
{
	t_string	b;
	char		*out;

	vec_init(&b);
	b.elem_size = 1;
	vec_push_str(&b, "${");
	vec_push_str(&b, (char *)mid);
	if (rlen > 0)
		vec_push_nstr(&b, rest, rlen);
	vec_push_str(&b, "}");
	out = expand_param_word(state, (char *)b.ctx, (int)b.len, false);
	if (!out)
		out = ft_strdup("");
	return (xfree(b.ctx), out);
}

/* ${!name<op...>}: bash resolves the indirection FIRST, then applies the
   operator to the TARGET parameter -- ${!ref-w} answers for the variable
   $ref names, not for ref. bash-completion 2.16 leans on it at every TAB:
   _comp_get_words does `printf -v "$ref" %s "${!ref-}..."` with ref set
   to "words[0]"-style element references, and rejecting the form as a
   bad substitution broke completion word assembly (#105, wave 2). The
   body is rebuilt as "${<target><op...>}" and re-expanded, so every
   operator and target shape runs through the machinery it already has.
   NULL means "not this form" (the ${!a[@]} keys and ${!pre*} prefix
   spellings fall through to their own handlers). A middle that is
   unset, empty or not a clean target is reported like bash's "invalid
   indirect expansion". */
char	*pf_indirect(t_shell *state, const char *s, int n, bool dq)
{
	int		nl;
	char	*mid;

	(void)dq;
	nl = pf_scan_scalar_name(s + 1, n - 1);
	if (nl <= 0 || (nl < n - 1 && ft_strchr("*@[", s[1 + nl])))
		return (NULL);
	mid = env_expand_n(state, (char *)s + 1, nl);
	if (!mid || !*mid || !pf_target_ok(mid))
		return (pf_bad_subst(state, s, n));
	return (pf_expand_rebuilt(state, mid, s + 1 + nl, n - 1 - nl));
}

/* The early ${...} forms, split from expand_param_format for the norm
   line budget only -- priority order unchanged: zsh dialect first (its
   ${(f)v} flag spellings would misparse as trims), then length, then
   indirection (bare or with an operator; pf_indirect returns NULL for
   the ${!a[@]} keys and ${!pre*} prefix forms so they fall through to
   their own handlers). NULL continues to the operator scans. */
char	*pf_head_dispatch(t_shell *state, const char *s, int slen, bool dq)
{
	char	*op;

	op = zsh_param(state, s, slen);
	if (op)
		return (op);
	op = zsh_dispatch(state, s, slen, false);
	if (op)
		return (op);
	if (s[0] == '#' && slen > 1)
		return (expand_strlen(state, s + 1, slen - 1));
	if (s[0] == '!' && slen > 1)
		return (pf_indirect(state, s, slen, dq));
	return (NULL);
}
