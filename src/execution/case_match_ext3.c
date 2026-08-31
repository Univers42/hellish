/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   case_match_ext3.c                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/30 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/30 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "libft.h"
#include "ft_glob.h"
#include "case_match.h"

/* Three small answers the rest of the extglob code kept spelling out for
** itself, and one of them was spelled wrong.
**
** WHAT A GROUP IS, in two dialects.  bash writes an operator in front of the
** paren -- `@(a|b)`, `+(a|b)` -- and only when `shopt -s extglob` is on.  zsh
** writes the bare paren, always: `_zsh_autosuggest_(bound|orig)_*` is one
** pattern, not a word and a subshell.  Both are the same thing to the
** matcher, so both come through xg_start and differ only in where the `(` is
** -- which is what xg_open exists to answer, so nothing downstream has to
** care which dialect wrote the group.
**
** WHY THE ZSH FORM DEMANDS A `|`.  A bare `(` is also how a subshell opens
** and how a function is declared.  `f() { ...; }` would become the single
** word `f()` and every zsh function definition would stop parsing.  Requiring
** a top-level alternative is what tells the two apart, and it is not a
** heuristic about what the text looks like: a parenthesised run with a `|` in
** it is not a function header in any shell.  `foo(bar)` -- a zsh group with
** one alternative -- is therefore NOT claimed, and stays the syntax error it
** is today.  That is deliberate: refusing it is loud, and claiming it would
** break `f()`.
*/

/* Where the group's `(` is.  bash puts an operator in front of it, zsh does
   not, so every caller that needs the paren or the alternatives asks here
   rather than assuming `p + 1`. */
const char	*xg_open(const char *p)
{
	if (*p == '(')
		return (p);
	return (p + 1);
}

/* The bytes a QUOTED pattern segment has to escape so the matcher reads them
   as text.
     `*?[\` were escaped and the group characters were not, so a quoted
   `"@(a)"` was still matched as a group: `case "@(a)" in "@(a)")` stopped
   matching itself the moment extglob was switched on, and `case a in "@(a)")`
   started matching when it must not.  Quoting is the only way to write a
   literal paren in a pattern, so losing it has no workaround. */
bool	xg_meta(char c)
{
	return (c == '*' || c == '?' || c == '[' || c == '\\'
		|| c == '(' || c == ')' || c == '|');
}

/* The byte length of the zsh alternation group at `at`, or 0.  Balance is
   xg_group_end's answer and "has an alternative" is xg_alt_end's, so this
   cannot drift from the code that later splits the group up -- a group the
   lexer claims and the matcher then reads as literal text is a pattern that
   silently never fires. */
int	xg_alt_group(const char *at)
{
	const char	*end;

	if (*at != '(' || !glob_zsh())
		return (0);
	end = xg_group_end(at);
	if (!end || *xg_alt_end(at + 1) != '|')
		return (0);
	return ((int)(end - at));
}

/* The lexer's two extra conditions, both about parens already spoken for.
**
**   at the START of a word   `( ls | wc )` is balanced and has a top-level
**                            `|`, so the shared test above calls it a group
**                            -- right about the bytes, catastrophically
**                            wrong about the line.
**   straight after an `=`    `x=(a|b c)` is an ARRAY, and it is an array in
**                            exactly the dialect that writes bare groups.
**                            Claiming it collapsed the assignment into one
**                            word and the array into one element.
**
** Only the lexer faces either question: by the time a pattern reaches the
** matcher it is already a word, and a filename segment has no subshell to be
** confused with.  That is why these live here rather than in xg_alt_group,
** and why the glob walker calls xg_alt_group directly. */
int	zsh_alt_ahead(const char *start, const char *at)
{
	if (at == start || at[-1] == '=')
		return (0);
	return (xg_alt_group(at));
}
