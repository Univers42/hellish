/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   glob_pattern.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/07 14:30:00 by dlesieur          #+#    #+#             */
/*   Updated: 2025/11/07 19:15:00 by dlesieur         ###   ########.fr       */
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

#ifdef HANDLE_MULTIBYTE
int glob_pattern_p(const char *pattern)
{
	size_t	n;
	wchar_t	*wpattern;
	int		r;

	if (MB_CUR_MAX == 1 || mbmbchar(pattern) == 0)
		return (internal_glob_pattern_p((uint8_t*)pattern));
	n = xdupmbstowcs (&wpattern, NULL, pattern);
	if (n == (size_t)-1)
		return (internal_glob_pattern_p((uint8_t*)pattern));
	r = internal_glob_wpattern_p(wpattern);
	free(wpattern);
	return (r);
}
#else
int glob_pattern_p(const char *pattern)
{
    return (internal_glob_pattern_p((unsigned char *)pattern));
}
#endif