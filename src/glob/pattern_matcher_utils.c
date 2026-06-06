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

/* Match '*': zero or more characters within a single path component. The
   leading-dot guard prevents "*.c" from matching ".hidden.c". The loop walks
   name forward and at each position tries to match the NEXT token (offset+1)
   against the remaining name -- classic backtracking. The finished_pattern
   check handles the case where '*' is the last token: if there's nothing more
   to match, any suffix works and we return offset+1 immediately at EOL. */
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

/* Match '?': exactly one non-NUL character (not a leading dot). If this is
   the last token in the segment (finished_pattern), the character after the
   matched one must be NUL. Otherwise recurse on offset+1 with the rest of
   the name. */
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

/* Match a bracket expression: the current character must satisfy the class
   (honoring BRACKET_NEGATED). Leading-dot and NUL guards come first.
   On success, if the bracket is the last token in the segment the next char
   must be NUL; otherwise recurse to match the rest. */
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
