/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   lex_glob_qual.c                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 23:33:59 by marvin            #+#    #+#             */
/*   Updated: 2026/01/09 23:33:59 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "lexer.h"
#include "ft_glob.h"

/* A zsh GLOB QUALIFIER group, claimed by the LEXER so `(` does not end the
** word:
**
**     content=("${extract_dir}"/[*](DNY2))
**
** Without this the `(` opens a subshell and the line is a syntax error --
** the glob layer never sees the pattern at all, so implementing qualifiers
** there was necessary and not sufficient.
**
** THREE CONDITIONS, all required, which is what keeps it from claiming
** anything else:
**
**   the zsh dialect is armed        -- in bash `[*](N)` is extglob syntax,
**                                      the same bytes in another language
**   the word is not empty           -- a `(` at the START of a word still
**                                      opens a subshell, so `(cd /; ls)`
**                                      is untouched
**   the group is qualifier letters  -- `ls foo(bar)` has a `b` and an `r`
**                                      in it, so it is not a qualifier and
**                                      falls through to the old behaviour
**
** Returns the byte length of the group, or 0 to leave the scanner alone. */
int	glob_qual_ahead(const char *start, const char *at)
{
	int	i;

	if (!glob_zsh() || at == start || *at != '(')
		return (0);
	i = 1;
	while (at[i] && at[i] != ')')
	{
		if (!ft_strchr("DN./@Y0123456789", at[i]))
			return (0);
		i++;
	}
	if (at[i] != ')' || i == 1)
		return (0);
	return (i + 1);
}

/* Does a `=(` here end the word?
**
** Only when the character before it is ALSO `=`. That single condition is
** what keeps zsh's two `=`-and-paren forms apart, and they are one character
** different:
**
**     a=(x y)     an ARRAY assignment   -- the `=` belongs to the assignment
**     a==(cmd)    assignment of a PROC SUB -- the second `=` starts it
**
** Without the check, `a=(foo bar)` lexed as the word `a` followed by a
** process substitution running `foo`, which is not a syntax error and not
** an array either: it silently ran the elements as a command. Every array
** literal in every zsh plugin went through that path.
**
** `=(cmd)` at the START of a word is not handled here -- the tokeniser
** pulls it out before the word scanner ever runs (try_parse_lexeme). */
bool	zsh_eqsub_break(const char *start, const char *at)
{
	return (glob_zsh() && at > start && at[0] == '='
		&& at[1] == '(' && at[-1] == '=');
}
