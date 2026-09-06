/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   mbchar2.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/02 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/02 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "mbchar.h"
#include "libft.h"
#include <stdlib.h>
#include <langinfo.h>

/* Stepping BACKWARDS through characters, for the suffix-trim scans
   (${v%pat}, ${v%%pat}): every candidate suffix has to start on a
   character boundary, or `?` matches the tail byte of an é and the trim
   cuts the character in half (issue #120).  Under UTF-8 a boundary is any
   byte that is not a continuation byte, which is a constant-time step;
   any other multibyte encoding walks forward from the start, which is what
   bash's own conversion to wide characters costs too. */

/* Byte offset of the character that ends just before offset i in s
   (i > 0), i.e. the previous character's start. */
size_t	mb_back(const char *s, size_t i)
{
	size_t	at;
	size_t	next;

	if (i == 0)
		return (0);
	if (MB_CUR_MAX == 1)
		return (i - 1);
	if (ft_strcmp(nl_langinfo(CODESET), "UTF-8") == 0)
	{
		i--;
		while (i > 0 && ((unsigned char)s[i] & 0xC0) == 0x80)
			i--;
		return (i);
	}
	at = 0;
	next = mb_len(s, i);
	while (at + next < i)
	{
		at += next;
		next = mb_len(s + at, i - at);
	}
	return (at);
}
