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

/*
** Match literal pattern tokens
*/
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

/*
** Main pattern matching dispatcher
*/
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
