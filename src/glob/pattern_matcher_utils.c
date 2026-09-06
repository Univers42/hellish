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
#include "mbchar.h"

/* Match '*': zero or more characters within a single path component. The
   leading-dot guard prevents "*.c" from matching ".hidden.c". The loop walks
   name forward and at each position tries to match the NEXT token (offset+1)
   against the remaining name -- classic backtracking. The finished_pattern
   check handles the case where '*' is the last token: if there's nothing more
   to match, any suffix works and we return offset+1 immediately at EOL. */
/* The leading-dot rule, in the matcher the DIRECTORY WALK uses.
**
** There are two matchers in this directory: glob_match.c answers "does this
** whole name match this pattern" for `case` and `[[ = ]]`, and this one
** drives the incremental walk. They implement the same rules, so a rule
** fixed in one and not the other is fixed nowhere that matters -- which is
** what happened first here: `shopt -s dotglob` started working for pattern
** MATCHING and still hid dotfiles when globbing a directory.
**
** Both now ask glob_dotglob(), which is also what zsh's `(D)` qualifier
** arms for the length of one walk. */
size_t	match_g_asterisk(char *name, t_vec_glob patt, size_t offset,
							bool first)
{
	size_t	res;

	if (first && *name == '.' && !glob_dotglob())
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

/* Match '?': exactly one non-NUL CHARACTER (not a leading dot) -- every
   byte of it under a multibyte locale (issue #120: `caf?` matches café,
   ${x%?} removes the whole é). If this is the last token in the segment
   (finished_pattern), what follows the matched character must be NUL.
   Otherwise recurse on offset+1 with the rest of the name. */
size_t	match_g_question(char *name, t_vec_glob patt, size_t offset,
							bool first)
{
	size_t	n;

	if (*name == '\0')
		return (0);
	if (first && *name == '.' && !glob_dotglob())
		return (0);
	n = mb_len0(name);
	if (finished_pattern(patt, offset))
	{
		if (name[n] == '\0')
			return (offset + 1);
		return (0);
	}
	return (matches_pattern(name + n, patt, offset + 1, false));
}

/* Match a bracket expression: the current character must satisfy the class
   (honoring BRACKET_NEGATED); a multibyte one is matched whole against the
   bracket's members. Leading-dot and NUL guards come first. On success, if
   the bracket is the last token in the segment the next char must be NUL;
   otherwise recurse to match the rest. */
size_t	match_g_bracket(char *name, t_vec_glob patt, size_t offset,
							bool first)
{
	t_glob	*g;
	size_t	n;

	if (*name == '\0')
		return (0);
	if (first && *name == '.' && !glob_dotglob())
		return (0);
	g = &((t_glob *)patt.ctx)[offset];
	n = mb_len0(name);
	if (n == 1 && !glob_char_in_class(*name, g))
		return (0);
	if (n > 1 && !glob_mb_in_class(name, n, g))
		return (0);
	if (finished_pattern(patt, offset))
	{
		if (name[n] == '\0')
			return (offset + 1);
		return (0);
	}
	return (matches_pattern(name + n, patt, offset + 1, false));
}
