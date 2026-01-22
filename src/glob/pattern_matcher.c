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
static size_t	match_g_literal(char *name, t_vec_glob patt, size_t offset,
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
	if (first && (!ft_strcmp(orig_name, "..") || !ft_strcmp(orig_name, "."))
		&& offset != patt.len && ((t_glob *)patt.ctx)[offset].ty != G_SLASH)
		return (0);
	return (matches_pattern(name, patt, offset, false));
}

/*
** Match asterisk - zero or more characters
*/
static size_t	match_g_asterisk(char *name, t_vec_glob patt, size_t offset,
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
static size_t	match_g_question(char *name, t_vec_glob patt, size_t offset,
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
static size_t	match_g_bracket(char *name, t_vec_glob patt, size_t offset,
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
