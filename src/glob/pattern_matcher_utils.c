/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   pattern_matcher_utils.c                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 11:14:29 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/25 19:42:28 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "glob_private.h"

/*
** Match asterisk - zero or more characters
*/
size_t	match_g_asterisk(char *name, t_vec_glob patt, size_t offset,
							bool first)
{
	size_t	res;

	if (first && *name == '.')
		return (0);
	while (*name)
	{
		if (!finished_pattern(patt, offset))
		{
			res = matches_pattern(name, patt, offset + 1, false);
			if (res)
				return (res);
		}
		name++;
	}
	if (finished_pattern(patt, offset))
		return (offset + 1);
	return (0);
}

/*
** Match question mark - exactly one character
*/
size_t	match_g_question(char *name, t_vec_glob patt, size_t offset,
							bool first)
{
	if (*name == '\0')
		return (0);
	if (first && *name == '.')
		return (0);
	if (finished_pattern(patt, offset))
	{
		if (name[1] == '\0')
			return (offset + 1);
		return (0);
	}
	return (matches_pattern(name + 1, patt, offset + 1, false));
}

/*
** Match bracket expression - character class
*/
size_t	match_g_bracket(char *name, t_vec_glob patt, size_t offset,
							bool first)
{
	t_glob	*g;

	if (*name == '\0')
		return (0);
	if (first && *name == '.')
		return (0);
	g = &((t_glob *)patt.ctx)[offset];
	if (!glob_char_in_class(*name, g))
		return (0);
	if (finished_pattern(patt, offset))
	{
		if (name[1] == '\0')
			return (offset + 1);
		return (0);
	}
	return (matches_pattern(name + 1, patt, offset + 1, false));
}
