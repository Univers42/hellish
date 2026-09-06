/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   case_match3.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/02 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/02 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "case_match.h"
#include "mbchar.h"
#include "libft.h"
#include <stdlib.h>
#include <wchar.h>
#include <wctype.h>

/* The multibyte members of a bracket expression, issue #120.  A range is
   compared by CODE POINT, which is bash's default (globasciiranges is on
   since bash 5.0), so `[à-ü]` covers what a reader expects and `[a-z]` does
   not swallow é.  Classes for a character that is not a single byte are
   answered by the wide-character tables, so `[[:alpha:]]` accepts é the
   way bash does under a UTF-8 locale. */

/* Decode the character at s (n bytes): its code point, or -1. */
static long	cm_decode(const char *s, size_t n)
{
	mbstate_t	st;
	wchar_t		wc;

	if (n == 1)
		return ((unsigned char)*s);
	ft_memset(&st, 0, sizeof(st));
	if (mbrtowc(&wc, s, n, &st) != n)
		return (-1);
	return ((long)wc);
}

/* One character each at lo and hi (any width): is c between them? */
bool	cm_in_range(const char *c, size_t n, const char *lo, const char *hi)
{
	long	v;
	long	a;
	long	b;

	v = cm_decode(c, n);
	a = cm_decode(lo, mb_len0(lo));
	b = cm_decode(hi, mb_len0(hi));
	if (v < 0 || a < 0 || b < 0)
		return (false);
	return (v >= a && v <= b);
}

/* The rarer half of the class table for wide characters. */
static bool	cm_class_has_w2(const char *name, int len, wint_t w)
{
	if (len == 5 && !ft_strncmp(name, "punct", 5))
		return (iswpunct(w) != 0);
	if (len == 5 && !ft_strncmp(name, "print", 5))
		return (iswprint(w) != 0);
	if (len == 5 && !ft_strncmp(name, "graph", 5))
		return (iswgraph(w) != 0);
	if (len == 5 && !ft_strncmp(name, "cntrl", 5))
		return (iswcntrl(w) != 0);
	if (len == 4 && !ft_strncmp(name, "word", 4))
		return (iswalnum(w) != 0 || w == L'_');
	return (false);
}

/* Does the character c (n bytes, n > 1) belong to the class name[0..len)?
   blank, ascii, digit and xdigit hold no multibyte member, like bash. */
bool	cm_class_has_w(const char *name, int len, const char *c, size_t n)
{
	long	v;

	v = cm_decode(c, n);
	if (v < 0)
		return (false);
	if (len == 5 && !ft_strncmp(name, "alpha", 5))
		return (iswalpha((wint_t)v) != 0);
	if (len == 5 && !ft_strncmp(name, "alnum", 5))
		return (iswalnum((wint_t)v) != 0);
	if (len == 5 && !ft_strncmp(name, "space", 5))
		return (iswspace((wint_t)v) != 0);
	if (len == 5 && !ft_strncmp(name, "upper", 5))
		return (iswupper((wint_t)v) != 0);
	if (len == 5 && !ft_strncmp(name, "lower", 5))
		return (iswlower((wint_t)v) != 0);
	return (cm_class_has_w2(name, len, (wint_t)v));
}

/* Where does the bracket expression starting at p (p points at '[') close?
** NULL when it never does -- and that is not an error, it is a character:
** POSIX says a '[' introducing no valid bracket expression is an ordinary
** '['. Both directions were wrong without this:
**
**     case "["  in [)     bash: matches.    here: did not
**     case "a"  in [abc)  bash: no match.   here: matched, on the 'a'
**
** The second is the dangerous one: a pattern that matches things it does
** not name. `${line#[}`, stripping a literal bracket off an INI section
** header, is the shape that found it.
**
** A ']' as the FIRST member is a literal member and not the close, so
** `[]abc]` is a four-character class. */
const char	*bracket_close(const char *p)
{
	const char	*q;

	q = p + 1;
	if (*q == '!' || *q == '^')
		q++;
	if (*q == ']')
		q++;
	while (*q && *q != ']')
	{
		if (q[0] == '[' && q[1] == ':')
			cm_class_skip(&q);
		else
			q++;
	}
	if (*q == ']')
		return (q);
	return (NULL);
}
