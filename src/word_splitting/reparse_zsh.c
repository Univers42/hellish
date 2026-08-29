/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   reparse_zsh.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/30 01:45:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/30 01:45:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "reparser_private.h"
#include "decomposer.h"

/* Unbraced `$+commands[pixz]` and `$commands[pixz]`.
**
** The braced spellings already work -- the expander handles the body of any
** ${...} -- so this is purely about the LEXICAL shape. zsh lets a `+` and a
** `[subscript]` join the name; bash does not, and there `$+commands[x]` is a
** dollar followed by literal text that must stay literal.
**
** The corpus writes it unbraced twelve times out of thirteen:
**
**     (( $+commands[pixz] )) && tar -I pixz -xvf "$f"
**
** Deliberately narrow. It claims a span only when the whole shape is
** present -- optional `+`, then a NAME, then a bracketed subscript -- so a
** bare `$+` or `$commands` with no subscript falls through to the ordinary
** scanner untouched, and no bash input can reach it because the dialect is
** checked first.
*/

/* Length of the `[...]` at `s`, brackets included, or 0.  Nesting is
   counted so `$commands[${x}]` finds its own close, and an unterminated
   subscript yields 0 rather than eating the rest of the word. */
static int	zr_sub_len(const char *s, int len, int i)
{
	int	depth;
	int	start;

	if (i >= len || s[i] != '[')
		return (0);
	depth = 0;
	start = i;
	while (i < len)
	{
		if (s[i] == '[')
			depth++;
		else if (s[i] == ']' && --depth == 0)
			return (i - start + 1);
		i++;
	}
	return (0);
}

/* The whole `+name[key]` / `name[key]` span at rp->i, or 0 when the shape is
   not there.  rp->i is left where it was; the caller advances it. */
static int	zr_span(t_reparser *rp)
{
	const char	*s;
	int			len;
	int			i;
	int			n;

	s = rp->current_token.start;
	len = rp->current_token.len;
	i = rp->i;
	if (i < len && s[i] == '+')
		i++;
	if (i >= len || !is_var_name_p1(s[i]))
		return (0);
	while (i < len && is_var_name_p2(s[i]))
		i++;
	n = zr_sub_len(s, len, i);
	if (!n)
		return (0);
	return (i + n - rp->i);
}

/* Claim the span as one TT_ENVVAR/TT_DQENVVAR subtoken, so the expander sees
   the whole `+commands[pixz]` as the body it already knows how to read.
   False leaves rp->i untouched for the ordinary scanner. */
bool	reparse_zsh_param(t_reparser *rp, int prev_start, t_tt tt)
{
	int	n;

	if (!reparse_zsh())
		return (false);
	n = zr_span(rp);
	if (!n)
		return (false);
	rp->i += n;
	push_subtoken_node(&rp->current_node, rp->current_token,
		create_interval(prev_start, rp->i), tt);
	return (true);
}
