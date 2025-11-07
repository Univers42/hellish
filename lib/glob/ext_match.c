/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ext_match.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/07 14:49:19 by dlesieur          #+#    #+#             */
/*   Updated: 2025/11/07 14:57:23 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "glob.h"

static int	ext_plus_star(char xc, char *s, char *se,
				char *p, char *pe, char *prest, int flags)
{
	char	*psub;
	char	*pnext;
	char	*srest;
	int	 m1;
	int	 m2;
	int	 xflags;

	/* '*': try zero matches first */
	if (xc == '*' && GMATCH(s, se, prest, pe, NULL, flags) == 0)
		return (0);
	for (psub = p + 1; ; psub = pnext)
	{
		pnext = PATSCAN(psub, pe, '|', flags);
		for (srest = s; srest <= se; srest++)
		{
			m1 = (GMATCH(s, srest, psub, pnext - 1, NULL, flags) == 0);
			if (m1)
			{
				xflags = (srest > s) ? (flags & ~(FNM_PERIOD | FNM_DOTDOT))
					: flags;
				m2 = (GMATCH(srest, se, prest, pe, NULL, xflags) == 0)
					|| (s != srest && GMATCH(srest, se, p - 1, pe, NULL, xflags) == 0);
			}
			else
				m2 = 0;
			if (m1 && m2)
				return (0);
		}
		if (pnext == prest)
			break;
	}
	return (FNM_NOMATCH);
}

static int	ext_q_at(char xc, char *s, char *se,
				char *p, char *pe, char *prest, int flags)
{
	char	*psub;
	char	*pnext;
	char	*srest;
	int	 xflags;

	/* '?' can match zero occurrences */
	if (xc == '?' && GMATCH(s, se, prest, pe, NULL, flags) == 0)
		return (0);
	for (psub = p + 1; ; psub = pnext)
	{
		pnext = PATSCAN(psub, pe, '|', flags);
		srest = (prest == pe) ? se : s;
		for ( ; srest <= se; srest++)
		{
			xflags = (srest > s) ? (flags & ~(FNM_PERIOD | FNM_DOTDOT)) : flags;
			if (GMATCH(s, srest, psub, pnext - 1, NULL, flags) == 0 &&
			    GMATCH(srest, se, prest, pe, NULL, xflags) == 0)
				return (0);
		}
		if (pnext == prest)
			break;
	}
	return (FNM_NOMATCH);
}

static int	ext_bang(char *s, char *se, char *p, char *pe, char *prest, int flags)
{
	char	*psub;
	char	*pnext;
	char	*srest;
	int	 m1;
	int	 xflags;

	for (srest = s; srest <= se; srest++)
	{
		m1 = 0;
		for (psub = p + 1; ; psub = pnext)
		{
			pnext = PATSCAN(psub, pe, '|', flags);
			if ((m1 = (GMATCH(s, srest, psub, pnext - 1, NULL, flags) == 0)))
				break;
			if (pnext == prest)
				break;
		}
		if (m1 == 0 && (flags & FNM_PERIOD) && *s == '.')
			return (FNM_NOMATCH);
		if (m1 == 0 && (flags & FNM_DOTDOT) && (SDOT_OR_DOTDOT(s)))
			return (FNM_NOMATCH);
		xflags = (srest > s) ? (flags & ~(FNM_PERIOD | FNM_DOTDOT)) : flags;
		if (m1 == 0 && GMATCH(srest, se, prest, pe, NULL, xflags) == 0)
			return (0);
	}
	return (FNM_NOMATCH);
}

int	extmatch(int xc, char *s, char *se, char *p, char *pe, int flags)
{
	char	*prest;

	if (p == NULL || pe == NULL)
		return (FNM_NOMATCH);
	/* scan subpattern starting after '(' if present */
	prest = PATSCAN(p + (*p == '('), pe, 0, flags);
	if (prest == NULL)
		return (STRCOMPARE(p - 1, pe, s, se));
	glob_recursion_depth++;
	if (xc == '+' || xc == '*')
		return (ext_plus_star((char)xc, s, se, p, pe, prest, flags));
	if (xc == '?' || xc == '@')
		return (ext_q_at((char)xc, s, se, p, pe, prest, flags));
	if (xc == '!')
		return (ext_bang(s, se, p, pe, prest, flags));
	return (FNM_NOMATCH);
}
