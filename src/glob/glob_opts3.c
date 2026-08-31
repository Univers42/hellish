/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   glob_opts3.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/21 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/07/21 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "glob_private.h"
#include "ft_glob.h"

/* `shopt -s extglob`, mirrored the same way. Both the lexer (so `@(a|b)` is
   one word rather than a syntax error) and the matcher consult it, and they
   must agree: a group that lexes as a word but matches literally would be a
   pattern that silently never fires. */

int	*glob_extglob_cell(void)
{
	static int	on;

	return (&on);
}

int	glob_extglob(void)
{
	return (*glob_extglob_cell());
}

/* `shopt -s nocaseglob` (also `pretty on case-blind-glob`, also zsh's
** `setopt nocaseglob`). It was stored and read by nothing at all: the bit
** flipped, `shopt nocaseglob` answered `on`, and `echo *.TXT` went on
** matching nothing. #72 phase 5 -- an option that reports success and does
** not exist is worse than one that is missing, because there is nothing to
** notice.
**
** Filename matching only, which is why it lives here and not in case_match:
** `case $x in A) ;; esac` stays case-SENSITIVE in bash whatever nocaseglob
** says, and one shared matcher behind one shared flag would have quietly
** changed that too.
**
** Known edge, stated rather than hidden: an extglob alternative inside a
** filename pattern -- `@(FOO|bar)` -- is matched by case_match and so is
** NOT folded. The plain forms (*, ?, [..], literal runs) all are.
*/

int	*glob_nocase_cell(void)
{
	static int	on;

	return (&on);
}

int	glob_nocase(void)
{
	return (*glob_nocase_cell());
}

/* The comparison every literal run in a pattern goes through: byte-exact
   normally, case-folded when nocaseglob is on. */
int	glob_ncmp(const char *a, const char *b, size_t n)
{
	size_t	i;
	int		x;
	int		y;

	if (!glob_nocase())
		return (ft_strncmp((char *)a, (char *)b, n));
	i = 0;
	while (i < n)
	{
		x = ft_tolower((unsigned char)a[i]);
		y = ft_tolower((unsigned char)b[i]);
		if (x != y || !x)
			return (x - y);
		i++;
	}
	return (0);
}
