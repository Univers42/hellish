/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   case_match.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/02 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/02 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "libft.h"

/* Match the single char `c` against a [...] bracket expression starting at
   *pp (which points at '['). Advances *pp past the closing ']'. */
static bool	match_bracket(char c, const char **pp)
{
	const char	*p;
	bool		neg;
	bool		hit;

	p = *pp + 1;
	neg = (*p == '!' || *p == '^');
	p += neg;
	hit = false;
	while (*p && *p != ']')
	{
		if (p[1] == '-' && p[2] && p[2] != ']')
		{
			hit = hit || ((unsigned char)c >= (unsigned char)p[0]
					&& (unsigned char)c <= (unsigned char)p[2]);
			p += 3;
		}
		else
		{
			hit = hit || (c == *p);
			p++;
		}
	}
	*pp = p + (*p == ']');
	return (hit != neg);
}

/* fnmatch-style glob match used by `case` patterns: '*' '?' '[..]' and '\'. */
bool	case_match(const char *s, const char *p)
{
	while (*p)
	{
		if (*p == '*')
		{
			while (*p == '*')
				p++;
			if (!*p)
				return (true);
			while (*s)
				if (case_match(s++, p))
					return (true);
			return (case_match(s, p));
		}
		if (*p == '?' && *s)
			(s++, p++);
		else if (*p == '[' && *s && match_bracket(*s, &p))
			s++;
		else if (*p == '\\' && p[1] == *s && *s)
			(s++, p += 2);
		else if (*p != '[' && *p != '?' && *p != '\\' && *p == *s)
			(s++, p++);
		else
			return (false);
	}
	return (*s == '\0');
}
