/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   lex_extglob.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/30 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/30 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "lexer.h"
#include "case_match.h"
#include "ft_glob.h"

int	glob_qual_ahead(const char *start, const char *at);

/* The lexer half of `shopt -s extglob`.
**
** `@(a|b)` is a PATTERN, but `(` is a shell metacharacter, so without this
** the word ended at the paren and the parser saw a subshell where a case
** label was expected: "syntax error near unexpected token `('".  The option
** could be switched on and reported on while every pattern that needs it
** was a parse error -- the loudest possible way for a feature to not exist,
** and still a lie in `shopt`'s output.
**
** This is the same hook zsh's glob qualifiers use (glob_qual_ahead): a
** parenthesised run that belongs to the WORD rather than opening a subshell,
** swallowed whole so the group's `|` and `)` never reach the grammar.
** Gated on the option, so a bash script that writes `x=@(1)` and means a
** literal at-sign keeps meaning that.
*/

/* How many bytes of extglob group start at `at`, or 0 if there is none.
   Delegates the balance scan to xg_group_end so the lexer and the matcher
   agree byte for byte on where a group ends -- if they disagreed, a group
   could lex as a word and then match as a literal, which is a pattern that
   silently never fires. */
int	extglob_ahead(const char *at)
{
	const char	*e;

	if (!glob_extglob() || !at[0] || !ft_strchr("?*+@!", at[0])
		|| at[1] != '(')
		return (0);
	e = xg_group_end(at + 1);
	if (!e)
		return (0);
	return ((int)(e - at));
}

/* The parenthesised runs that belong to a WORD rather than opening a
   subshell: a zsh glob qualifier at the end of a word, a zsh alternation
   anywhere but the front of one, and an extglob group anywhere in one. All
   are swallowed whole so their parens never reach the grammar; folded into
   one call so parse_lexeme's loop asks the question once. Returns the byte
   count to skip, or 0.
     The qualifier is asked FIRST and that ordering is load-bearing: `(N)`
   and `(a|b)` are both parenthesised runs and only the letters inside tell
   them apart, so the narrower test has to get there first or a qualifier
   that happens to contain a `|` would be matched as a pattern. */
int	word_group_ahead(const char *start, const char *at)
{
	int	n;

	n = glob_qual_ahead(start, at);
	if (n)
		return (n);
	n = zsh_alt_ahead(start, at);
	if (n)
		return (n);
	return (extglob_ahead(at));
}
