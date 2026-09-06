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
#include "mbchar.h"

/* Match a G_LITERAL token against the start of `name`. Returns the number of
   characters consumed (g->len) on success, or -1 on mismatch. The caller
   advances the name pointer by that amount. A literal can span multiple
   characters (e.g. the token for "hello" has len=5). */
int	match_literal(const char *name, t_glob *g)
{
	if (ft_strncmp(name, g->start, g->len) != 0)
		return (-1);
	return (g->len);
}

/* The leading-dot rule, in the three places a wildcard can meet one.
**
** POSIX excludes hidden files from wildcards unless the pattern itself
** starts with a dot -- and `shopt -s dotglob` turns that off, which is what
** glob_dotglob() answers.
**
** That check was MISSING: the cell was mirrored from state->shopt by the
** shopt builtin and by `pretty`, and then nothing ever read it, so
** `shopt -s dotglob; echo [*]` silently kept hiding dotfiles. A bash-parity
** bug of its own, found because zsh's `(D)` qualifier needs the same switch
** and there was nothing to switch. */

/* Match '?': exactly one non-NUL CHARACTER -- all of its bytes under a
   multibyte locale (issue #120: `caf?` has to match café). */
int	match_question(const char *name, bool is_first)
{
	if (*name == '\0')
		return (-1);
	if (is_first && *name == '.' && !glob_dotglob())
		return (-1);
	return ((int)mb_len0(name));
}

/* Match a G_BRACKET token: the current character must be in the pre-expanded
   char_set (or excluded from it if BRACKET_NEGATED). The same leading-dot
   rule applies as for '?': hidden files are never matched by a bracket that
   appears at the start of a path component. A multibyte character is
   matched as a whole against the bracket's members (glob_mb_in_class). */
int	match_bracket(const char *name, t_glob *g, bool is_first)
{
	size_t	n;

	if (*name == '\0')
		return (-1);
	if (is_first && *name == '.' && !glob_dotglob())
		return (-1);
	n = mb_len0(name);
	if (n > 1)
	{
		if (glob_mb_in_class(name, n, g))
			return ((int)n);
		return (-1);
	}
	if (!glob_char_in_class(*name, g))
		return (-1);
	return (1);
}

/* Match '*': try to match the remainder of the pattern starting at offset+1
   at every possible position in name. The loop walks name forward one char
   at a time and tries glob_match_at at each position -- classic backtracking.
   Leading-dot guard fires first: a '*' at the start of a component never
   matches a hidden file (".*" would need the literal dot in the pattern).
   The final call after the loop handles the empty-suffix case (* matches
   the empty string, so the rest of the pattern must match at EOL). */
bool	match_asterisk_recursive(const char *name, t_vec_glob *pattern,
									size_t offset, bool is_first)
{
	if (is_first && *name == '.' && !glob_dotglob())
		return (false);
	while (*name)
	{
		if (glob_match_at(name, pattern, offset + 1))
			return (true);
		name++;
	}
	return (glob_match_at(name, pattern, offset + 1));
}

/* Top-level match entry point: does `name` match the entire `pattern`?
   The pattern must not contain G_SLASH tokens (each path component is matched
   separately by the directory walker). Simply delegates to glob_match_at with
   offset 0. */
bool	glob_match(const char *name, t_vec_glob *pattern)
{
	return (glob_match_at(name, pattern, 0));
}
