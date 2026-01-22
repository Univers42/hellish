/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   glob_match.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/15 16:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/22 10:36:35 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "glob_private.h"

/*
** Match a literal pattern segment against a name
** Returns number of characters matched, or -1 on failure
*/
static int	match_literal(const char *name, t_glob *g)
{
	if (ft_strncmp(name, g->start, g->len) != 0)
		return (-1);
	return (g->len);
}

/*
** Match a question mark (single character, not leading dot)
*/
static int	match_question(const char *name, bool is_first)
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
static int	match_bracket(const char *name, t_glob *g, bool is_first)
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
static bool	match_asterisk_recursive(const char *name, t_vec_glob *pattern,
									size_t offset, bool is_first)
{
	/* First char cannot be '.' when asterisk is at start of segment */
	if (is_first && *name == '.')
		return (false);
	/* Try matching rest of pattern at current position */
	while (*name)
	{
		if (glob_match_at(name, pattern, offset + 1))
			return (true);
		name++;
	}
	/* Also try matching at end (asterisk matches empty at end) */
	return (glob_match_at(name, pattern, offset + 1));
}

/*
** Match pattern starting at given offset against name
** Returns true if entire remaining pattern matches
*/
bool	glob_match_at(const char *name, t_vec_glob *pattern, size_t offset)
{
	t_glob	*g;
	int		consumed;
	bool	is_first;

	/* Base case: consumed all pattern tokens */
	if (offset >= pattern->len)
		return (*name == '\0');
	g = &((t_glob *)pattern->ctx)[offset];
	is_first = (offset == 0 || ((t_glob *)pattern->ctx)[offset - 1].ty == G_SLASH);
	if (g->ty == G_SLASH)
	{
		/* Slash should not appear in filename matching */
		return (false);
	}
	else if (g->ty == G_LITERAL)
	{
		consumed = match_literal(name, g);
		if (consumed < 0)
			return (false);
		return (glob_match_at(name + consumed, pattern, offset + 1));
	}
	else if (g->ty == G_QUESTION)
	{
		consumed = match_question(name, is_first);
		if (consumed < 0)
			return (false);
		return (glob_match_at(name + consumed, pattern, offset + 1));
	}
	else if (g->ty == G_BRACKET)
	{
		consumed = match_bracket(name, g, is_first);
		if (consumed < 0)
			return (false);
		return (glob_match_at(name + consumed, pattern, offset + 1));
	}
	else if (g->ty == G_ASTERISK)
	{
		/* Asterisk: try matching zero or more characters */
		return (match_asterisk_recursive(name, pattern, offset, is_first));
	}
	return (false);
}

/*
** Match a complete pattern against a filename
** Pattern should not contain slashes (single path component)
*/
bool	glob_match(const char *name, t_vec_glob *pattern)
{
	return (glob_match_at(name, pattern, 0));
}
