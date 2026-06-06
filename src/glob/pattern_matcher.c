/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   pattern_matcher.c                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/15 16:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/15 16:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "glob_private.h"

/* Match one or more consecutive G_LITERAL tokens against the start of `name`.
   The while loop consumes a run of literals without recursing for each byte,
   which is the common fast path (most patterns are mostly literal text). On
   any mismatch return 0 immediately. If the match reaches a segment boundary
   (finished_pattern), the name must also be exhausted there. Otherwise we
   recurse to handle the next token type after the literal run. */
size_t	match_g_literal(char *name, t_vec_glob patt, size_t offset,
							bool first)
{
	t_glob	curr;
	char	*orig_name;

	orig_name = name;
	while (offset < patt.len && ((t_glob *)patt.ctx)[offset].ty == G_LITERAL)
	{
		curr = ((t_glob *)patt.ctx)[offset];
		if (ft_strncmp(curr.start, name, curr.len) != 0)
			return (0);
		if (finished_pattern(patt, offset))
		{
			if (name[curr.len] == 0)
				return (offset + 1);
			return (0);
		}
		offset++;
		name += curr.len;
	}
	(void)first;
	(void)orig_name;
	return (matches_pattern(name, patt, offset, false));
}

/* Main dispatcher: look at the current token type and delegate to the matching
   function. Returns the token-offset past the last matched token on success,
   or 0 on failure. The offset returned (not byte count) lets the caller know
   how far the pattern was consumed -- the directory walker uses this to detect
   partial matches that require descending into subdirectories. */
size_t	matches_pattern(char *name, t_vec_glob patt, size_t offset, bool first)
{
	t_glob	curr;

	if (offset >= patt.len)
		return (0);
	curr = ((t_glob *)patt.ctx)[offset];
	if (curr.ty == G_LITERAL)
		return (match_g_literal(name, patt, offset, first));
	else if (curr.ty == G_ASTERISK)
		return (match_g_asterisk(name, patt, offset, first));
	else if (curr.ty == G_QUESTION)
		return (match_g_question(name, patt, offset, first));
	else if (curr.ty == G_BRACKET)
		return (match_g_bracket(name, patt, offset, first));
	return (0);
}
