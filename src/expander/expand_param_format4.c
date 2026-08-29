/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   expand_param_format4.c                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/03/14 09:30:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/03/14 09:30:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"

/* Length of the parameter name at the head of a ${...} body, for the
   trim and substitution scanners: one of the SCALAR specials, a digit-run
   positional, or an identifier.  0 when no name starts the body.
     The scalar set is "-?$!" and the omissions are deliberate.  @ and * are
   out because they expand to a field LIST — trimming those is per-element
   work the scalar evaluators below cannot do, so ${@%x} keeps routing to
   bad substitution exactly as before.  # is out because it is unreachable:
   expand_param_format claims a leading '#' as the length form first.
     Without the specials here, `${-#*e}` — the idiom every nvm.sh-style rc
   file uses to test a flag in $- — died as a bad substitution, which is
   fatal (127) in a non-interactive shell. */
static int	pf_scan_scalar_name(const char *s, int slen)
{
	int	i;

	if (slen > 0 && s[0] && ft_strchr("-?$!", s[0]))
		return (1);
	i = 0;
	if (i < slen && ft_isdigit((unsigned char)s[i]))
		while (i < slen && ft_isdigit((unsigned char)s[i]))
			i++;
	else if (i < slen && (s[i] == '_' || ft_isalpha((unsigned char)s[i])))
	{
		i++;
		while (i < slen && (s[i] == '_' || ft_isalnum((unsigned char)s[i])))
			i++;
	}
	return (i);
}

/* Scan the ${...} body for a # / ## or % / %% operator that follows the
   variable name.  Sets *name_len to the length of the name prefix so the
   caller knows where the operator starts.  Returns a pointer to the first
   operator character, or NULL if no trim operator is present. */
static const char	*find_trim_op(const char *s, int slen, int *name_len)
{
	int	i;

	i = pf_scan_scalar_name(s, slen);
	if (i <= 0)
		return (NULL);
	*name_len = i;
	if (i < slen && (s[i] == '%' || s[i] == '#'))
		return (&s[i]);
	return (NULL);
}

/* Scan the ${...} body for a / operator after the variable name.  The
   difference between ${n/p/r} and ${n//p/r} (global replace) is detected
   later by expand_subst, not here.  Returns a pointer to the first '/', or
   NULL if no substitution operator is present. */
static const char	*find_subst_op(const char *s, int slen, int *name_len)
{
	int	i;

	i = pf_scan_scalar_name(s, slen);
	if (i <= 0)
		return (NULL);
	*name_len = i;
	if (i < slen && s[i] == '/')
		return (&s[i]);
	return (NULL);
}

/* The lower-priority ${...} forms tried after the operator and trim/subst
   scans: transforms (@Q family), case conversion (^ , ~), substrings, then
   the plain-parameter validity gate.  NULL keeps the "plain $v, caller
   falls through" contract.  Split out of expand_param_format only to stay
   inside the norm line budget — the priority order is unchanged. */
static char	*pf_tail_dispatch(t_shell *state, const char *s, int slen)
{
	int		name_len;
	char	xop;

	if (find_xform_op(s, slen, &name_len, &xop))
		return (expand_xform(state, s, name_len, xop));
	if (find_case_op(s, slen, &name_len))
		return (expand_case(state, s, slen, name_len));
	if (pf_find_substr(s, slen, &name_len))
		return (expand_substr(state, s, slen, name_len));
	if (!pf_valid_plain(s, slen))
		return (pf_bad_subst(state, s, slen));
	return (NULL);
}

/* Top-level ${...} dispatcher.  `s` is the raw text between the braces,
   `slen` its byte count.  Priority order — first match wins:
     ${#v}      → length of v                  (expand_strlen)
     ${v:-w}    → default/alt operators        (find_param_op → expand_param_op)
     ${v#p} etc → trim operators               (find_trim_op → expand_trim)
     ${v/p/r}   → substitution                 (find_subst_op → expand_subst)
     ${v:off}   → substring                    (pf_find_substr → expand_substr)
     plain $v   → returns NULL (caller falls through to env_expand_n)
   Anything that matches none of these and is not a plain parameter is a
   bad substitution (127, fatal when non-interactive — bash parity).
   Returns a freshly allocated string, or NULL if it is just a plain name. */
char	*expand_param_format(t_shell *state, const char *s, int slen, bool dq)
{
	const char	*op;
	int			name_len;
	t_pe_op		o;

	if (slen <= 0)
		return (pf_bad_subst(state, s, slen));
	op = zsh_param(state, s, slen);
	if (op)
		return ((char *)op);
	if (s[0] == '#' && slen > 1)
		return (expand_strlen(state, s + 1, slen - 1));
	if (s[0] == '!' && slen > 1 && pf_is_indirect(s + 1, slen - 1))
		return (expand_indirect(state, s + 1, slen - 1));
	if (find_param_op(s, slen, &o))
		return (o.dq = dq, expand_param_op(state, o));
	op = find_trim_op(s, slen, &name_len);
	if (!op)
		op = find_subst_op(s, slen, &name_len);
	if (op)
		return (pf_trim_or_subst(state,
				pf_make_ctx(s, slen, op, name_len)));
	return (pf_tail_dispatch(state, s, slen));
}
