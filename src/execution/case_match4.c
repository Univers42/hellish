/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   case_match4.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/14 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/09/14 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "libft.h"
#include "case_match.h"

/* The two ways in, both landing on the same cm_run.
**
** case_match_n is the one the matcher itself uses: an extglob alternative,
** the first `cut` bytes of a subject, the pattern held by one G_EXTGLOB
** token -- each is a slice of something longer, and asking about it used to
** mean ft_strndup'ing it first.  Those copies were the whole cost of the
** thing: every `|` of every alternative at every cut at every position,
** each copy the length of the prefix it described.
**
** case_match is the ordinary whole-string entry -- `case`, `[[ == ]]`,
** ${v#p} -- and is now just the slice that runs to each NUL.
*/

bool	case_match_n(const char *s, size_t slen, const char *p, size_t plen)
{
	t_cmp	m;

	m.s = s;
	m.se = s + slen;
	m.p = p;
	m.pe = p + plen;
	return (cm_run(m));
}

bool	case_match(const char *s, const char *p)
{
	return (case_match_n(s, ft_strlen(s), p, ft_strlen(p)));
}
