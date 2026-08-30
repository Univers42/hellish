/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   expand_zsh_sub.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/29 20:40:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/29 20:40:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"
#include "env.h"

/* A subscript on a nested expansion: `${${(s: :)$(git version)}[3]}`.
**
** oh-my-zsh's git plugin opens with exactly that line, and it is the shape
** the whole idiom takes -- split something, then take the nth piece --
** so without it the most-used plugin in the corpus stops at line 3.
**
** ZSH ARRAYS ARE 1-BASED.  [1] is the first element, and a plugin reading
** [3] of `git version 2.4.5` wants the version.  Reading that as a C index
** would return the wrong field and nothing would report an error, which is
** why the conversion happens once, here, rather than at each caller.
*/

/* Length of a trailing [...] subscript including both brackets, or 0.  Only
   the outermost pair counts: the scan starts at the closing bracket and
   walks back to its match, so `${...}[$#a]` is one subscript and not two. */
int	zn_sub_len(const char *s, int slen)
{
	int	depth;
	int	i;

	if (slen < 3 || s[slen - 1] != ']')
		return (0);
	depth = 0;
	i = slen - 1;
	while (i > 0)
	{
		if (s[i] == ']')
			depth++;
		else if (s[i] == '[' && --depth == 0)
			return (slen - i);
		i--;
	}
	return (0);
}

/* Pick one element of an encoded array, 1-based.  Out of range is the empty
   string, matching zsh -- and matching what ${arr[99]} already does here. */
static char	*zn_pick(const char *enc, long n)
{
	char	*e;

	if (n < 0)
		n += arr_count(enc) + 1;
	if (n < 1)
		return (ft_strdup(""));
	e = arr_get_idx(enc, n - 1);
	if (!e)
		return (ft_strdup(""));
	return (e);
}

/* Subscripting a SCALAR takes a character, not an element -- `${${(U)x}[1]}`
   is the first letter.  Same 1-based counting, which is the part that would
   be silently off by one if this fell through to "the whole string". */
static char	*zn_pick_char(const char *v, long n)
{
	if (n < 0)
		n += (long)ft_strlen(v) + 1;
	if (n < 1 || (size_t)n > ft_strlen(v))
		return (ft_strdup(""));
	return (ft_strndup(v + n - 1, 1));
}

/* A subscript on a plain SCALAR parameter, `${x[1]}`.
**
** bash answers the whole value at [0] and nothing anywhere else; zsh takes
** one CHARACTER, counting from 1, so `${x[1]}` of "hello" is "h".  Letting
** bash's rule stand under the zsh dialect would hand back the ENTIRE STRING
** for [1] -- an answer that looks like it worked.
*/
char	*zn_scalar_pick(t_shell *state, const char *val, long sub)
{
	if (!val)
		return (NULL);
	if (zsh_arrays(state))
		return (zn_pick_char(val, sub));
	if (sub == 0)
		return (ft_strdup(val));
	return (NULL);
}

/* Apply the trailing subscript, if there is one, to what the nested
   expansion produced.  Consumes `enc` and returns an owned value.
     [@] and [*] are not a pick: they mean "keep this an array", which
   zf_nested has already acted on, so the encoded value passes through. */
char	*zn_subscript(t_shell *state, char *enc, const char *s, int slen)
{
	int		n;
	char	*idx;
	char	*out;

	n = zn_sub_len(s, slen);
	if (n == 0 || !enc)
		return (enc);
	if (n == 3 && (s[slen - 2] == '@' || s[slen - 2] == '*'))
		return (enc);
	idx = arith_expand(state, s + slen - n + 1, n - 2);
	if (arr_is(enc))
		out = zn_pick(enc, ft_atol(idx));
	else
		out = zn_pick_char(enc, ft_atol(idx));
	xfree(idx);
	xfree(enc);
	return (out);
}
