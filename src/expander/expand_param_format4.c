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

/* Scan the ${...} body for a # / ## or % / %% operator that follows the
   variable name.  Sets *name_len to the length of the name prefix so the
   caller knows where the operator starts.  Returns a pointer to the first
   operator character, or NULL if no trim operator is present. */
static const char	*find_trim_op(const char *s, int slen, int *name_len)
{
	int	i;

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
	else
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
	else
		return (NULL);
	*name_len = i;
	if (i < slen && s[i] == '/')
		return (&s[i]);
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
char	*expand_param_format(t_shell *state, const char *s, int slen)
{
	const char	*op;
	int			name_len;
	t_pe_op		o;

	if (slen <= 0)
		return (pf_bad_subst(state, s, slen));
	if (s[0] == '#' && slen > 1)
		return (expand_strlen(state, s + 1, slen - 1));
	if (find_param_op(s, slen, &o))
		return (expand_param_op(state, o));
	op = find_trim_op(s, slen, &name_len);
	if (!op)
		op = find_subst_op(s, slen, &name_len);
	if (op)
		return (pf_trim_or_subst(state,
				pf_make_ctx(s, slen, op, name_len)));
	if (pf_find_substr(s, slen, &name_len))
		return (expand_substr(state, s, slen, name_len));
	if (!pf_valid_plain(s, slen))
		return (pf_bad_subst(state, s, slen));
	return (NULL);
}
