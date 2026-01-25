/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   glob_match.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/15 16:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/25 19:42:30 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "glob_private.h"

/*
** Match a literal pattern segment against a name
** Returns number of characters matched, or -1 on failure
*/
int	match_literal(const char *name, t_glob *g)
{
	if (ft_strncmp(name, g->start, g->len) != 0)
		return (-1);
	return (g->len);
}

/*
** Match a question mark (single character, not leading dot)
*/
int	match_question(const char *name, bool is_first)
{
	if (*name == '\0')
		return (-1);
	if (is_first && *name == '.')
		return (-1);
	return (1);
}

/*
** Match a bracket expression
*/
int	match_bracket(const char *name, t_glob *g, bool is_first)
{
	if (*name == '\0')
		return (-1);
	if (is_first && *name == '.')
		return (-1);
	if (!glob_char_in_class(*name, g))
		return (-1);
	return (1);
}

/*
** Recursive matching for asterisk
** Try to match rest of pattern at each position
*/
bool	match_asterisk_recursive(const char *name, t_vec_glob *pattern,
									size_t offset, bool is_first)
{
	if (is_first && *name == '.')
		return (false);
	while (*name)
	{
		if (glob_match_at(name, pattern, offset + 1))
			return (true);
		name++;
	}
	return (glob_match_at(name, pattern, offset + 1));
}

/*
** Match a complete pattern against a filename
** Pattern should not contain slashes (single path component)
*/
bool	glob_match(const char *name, t_vec_glob *pattern)
{
	return (glob_match_at(name, pattern, 0));
}
