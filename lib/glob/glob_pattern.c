/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   glob_pattern.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/07 14:30:00 by dlesieur          #+#    #+#             */
/*   Updated: 2025/11/07 14:39:16 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "glob.h"

/**
 * detects ? or * 
 * detects [...]
 * detects +[] +(), @(), !()
 * handles `\\`

 */
int	internal_glob_pattern_p(const char *pattern)
{
	const char	*p;
	char		c;
	int			bopen;
	int			bsquote;

	p = pattern;
	bopen = 0;
	bsquote = 0;
	while ((c = *p++) != '\0')
	{
		if (c == '?' || c == '*')
			return (1);
		if (c == '[')
			bopen++;
		else if (c == ']' && bopen)
			return (1);
		else if ((c == '+' || c == '@' || c == '!') && *p == '(')
			return (1);
		else if (c == '\\')
		{
			if (*p == '\0')
				return (0);
			p++;
			bsquote = 1;
		}
	}
	if (bsquote)
		return (2);
	return (0);
}


int glob_pattern_p(const char *pattern)
{
    return (internal_glob_pattern_p((unsigned char *)pattern));
}