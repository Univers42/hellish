/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   verif.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 22:54:52 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/20 23:58:59 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "reparser_private.h"

/* Bounds-safe character-pair testers used by the paren-depth scanner.
   All four functions are short because the logic is trivial, but inlining
   them would smear the same idx+1 bounds check all over the scanner loop.
   Keeping them here also makes it easy to add bounds-checking assertions
   during debugging without editing every call site. */
bool	is_double_open_paren(t_token t, int idx)
{
	return (idx + 1 < t.len && t.start[idx] == '(' && t.start[idx + 1] == '(');
}

bool	is_double_close_paren(t_token t, int idx)
{
	return (idx + 1 < t.len && t.start[idx] == ')' && t.start[idx + 1] == ')');
}

bool	is_open_paren(t_token t, int idx)
{
	return (t.start[idx] == '(');
}

bool	is_close_paren(t_token t, int idx)
{
	return (t.start[idx] == ')');
}

/* t_reparser wrapper: forwards to the token-level tester using rp's current
   position. Same motivation as the is_*_rp wrappers in utils2.c. */
bool	is_double_open_paren_rp(t_reparser *rp)
{
	return (is_double_open_paren(rp->current_token, rp->i));
}
