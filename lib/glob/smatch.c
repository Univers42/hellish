/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   smatch.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/07 14:42:49 by dlesieur          #+#    #+#             */
/*   Updated: 2025/11/07 14:46:50 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "glob.h"

#include <stddef.h>

static char	*parse_subbracket(char *s, int flags)
{
	(void)flags;
	while (*s && *s != ']' && *s != '\0')
		s++;
    if (*s == ']')
        return (s);
    return (NULL);
}

static void	handle_bracket(char **s, int *bnest, char **bfirst, int flags)
{
	char	*t;

	if (*bnest == 0)
	{
		*bfirst = *s + 1;
		if (**bfirst == '!' || **bfirst == '^')
			(*bfirst)++;
		(*bnest)++;
	}
	else if ((*s)[1] == ':' || (*s)[1] == '.' || (*s)[1] == '=')
	{
		t = parse_subbracket(*s + 1, flags);
		if (t)
			*s = t + 1;
	}
}

static int	handle_closing(char c, int *bnest, int *pnest, char **bfirst)
{
	if (c == ']' && *bnest && *bfirst)
	{
		(*bnest)--;
		*bfirst = NULL;
	}
	else if (c == '(' && *bnest == 0)
		(*pnest)++;
	else if (c == ')' && *bnest == 0)
	{
		(*pnest)--;
		if (*pnest < 0)
			return (1);
	}
	return (0);
}

char	*pattern_scan(char *str, char *end, int delim, int flags)
{
	int		pnest;
	int		bnest;
	int		skip;
	char	*bfirst;
	char	c;

	pnest = 0;
	bnest = 0;
	skip = 0;
	bfirst = NULL;
	if (str == end)
		return (NULL);
	while ((c = *str))
	{
		if (str >= end)
			return (str);
		if (skip)
			skip = 0;
		else if (c == '\\' && !(flags & 1))
			skip = 1;
		else if (c == '[')
			handle_bracket(&str, &bnest, &bfirst, flags);
		else if (handle_closing(c, &bnest, &pnest, &bfirst))
			return (++str);
		else if (c == '|' && bnest == 0 && pnest == 0 && delim == '|')
			return (++str);
		str++;
	}
	return (NULL);
}
