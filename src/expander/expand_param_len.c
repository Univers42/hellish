/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   expand_param_len.c                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/30 03:40:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/30 03:40:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"
#include "env.h"
#include "parena.h"
#include "mbchar.h"

/* The ${#...} family.  Together in one file because the three of them are
   one question -- "how long is this?" -- asked of a scalar, an array and an
   element, and the answers differ by DIALECT as well as by kind: on an array
   bash measures element 0 and zsh counts the elements. */

/* ${#a} on an ARRAY is the length of its first element, bash's rule.  The
   value in the environment is the encoded array, so the element has to be
   decoded out of it — and arr_get_idx hands back a fresh copy, which is why
   this measures and then releases rather than assigning over `val`. */
static char	*strlen_of_first(char *val)
{
	char	*elem;
	char	*result;

	elem = arr_get_idx(val, 0);
	if (!elem)
		return (ft_strdup("0"));
	result = ft_itoa((int)mb_count(elem, ft_strlen(elem)));
	xfree(elem);
	return (result);
}

/* ${#a[lo,hi]} counts the ELEMENTS the slice covers, not the width of the
   string it expands to: zsh answers 2 for `a[2,3]`, where the joined
   "two three" is nine characters.  NULL when the subscript is not a slice
   (or the dialect is off), which leaves every other ${#...} shape alone. */
static char	*strlen_of_slice(t_shell *state, const char *s, int slen)
{
	char	*br;
	char	*val;
	t_slice	r;
	int		nl;

	br = ft_strnchr((char *)s, '[', slen);
	if (!br || slen < 1 || s[slen - 1] != ']')
		return (NULL);
	nl = (int)(br - s);
	val = env_expand_n(state, (char *)s, nl);
	r = zsh_slice_bounds(state, s + nl + 1, slen - nl - 2, val);
	if (r.lo == SLICE_NONE)
		return (NULL);
	return (ft_itoa((int)zsh_slice_len(r, zsh_slice_universe(val))));
}

/* ${#arr[@]} is the element count, ${#arr[i]} an element's length; the
   subscript token is expanded by expand_array_token first, so here we
   only need the name[body] shapes on the raw text. */
static char	*expand_strlen_arr(t_shell *state, const char *s, int slen)
{
	t_token	tok;
	int		n;

	tok = (t_token){.tt = TT_ENVVAR, .start = (char *)s, .len = slen};
	if (slen > 3 && s[slen - 1] == ']' && (s[slen - 2] == '@'
			|| s[slen - 2] == '*') && s[slen - 3] == '[')
	{
		tok.start = env_expand_n(state, (char *)s, slen - 3);
		if (assoc_is(tok.start))
			return (ft_itoa(assoc_count(tok.start)));
		if (tok.start && !arr_is(tok.start))
			return (ft_itoa(1));
		return (ft_itoa(arr_count(tok.start)));
	}
	if (!expand_array_token(state, &tok, false))
		return (NULL);
	n = (int)mb_count(tok.start, (size_t)tok.len);
	if (tok.allocated)
		parena_free((char *)tok.start);
	return (ft_itoa(n));
}

/* ${#param} — return the length of the variable's value as a decimal string.
   An unset variable counts as length 0 rather than an error (unless set -u).
   The result is always a fresh malloc'd string (ft_itoa allocates).
   Length is in CHARACTERS under a multibyte locale (mb_count), bytes in
   the C locale -- bash's rule, issue #120.

   On an ARRAY the two dialects answer different questions: bash gives the
   length of element 0, zsh gives the ELEMENT COUNT.  `[[ $#stack -gt 0 ]]`
   -- a plugin asking whether its stack is empty -- is true under bash's
   reading for a one-element stack holding "" and false for one holding "/",
   which is the answer inverted, so the split is on zsh_arrays() rather than
   on a guess about which the caller meant. */
char	*expand_strlen(t_shell *state, const char *s, int slen)
{
	char	*val;
	char	*result;

	result = strlen_of_slice(state, s, slen);
	if (result)
		return (result);
	result = zsh_strlen(state, s, slen);
	if (result)
		return (result);
	if (ft_strnchr((char *)s, '[', slen))
	{
		result = expand_strlen_arr(state, s, slen);
		if (result)
			return (result);
	}
	val = pf_get_var_value(state, s, slen);
	if (!val)
		return (ft_strdup("0"));
	if (assoc_is(val) && zsh_arrays(state))
		return (ft_itoa(assoc_count(val)));
	if (arr_is(val) && zsh_arrays(state))
		return (ft_itoa(arr_count(val)));
	if (arr_is(val))
		return (strlen_of_first(val));
	return (ft_itoa((int)mb_count(val, ft_strlen(val))));
}
