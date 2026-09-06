/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   mbchar.c                                           :+:      :+:    :+:   */
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
#include <wchar.h>
#include <wctype.h>

/* Multibyte-aware string arithmetic, issue #120.
**
** bash counts CHARACTERS in a multibyte locale -- ${#var}, ${var:off:len},
** `?` and `[...]` in a pattern, read -n -- and bytes in the C locale, and
** an invalid or truncated sequence counts as one character one byte wide,
** which is also how bash steps past junk.  hellish counted bytes
** everywhere, so "café" measured 5, ${x%?} cut é in half, and `caf?` did
** not match it.  UTF-8 is stateless, so every call starts from a zeroed
** conversion state.  The single-byte and plain-ASCII cases never reach
** mbrtowc: MB_CUR_MAX is checked first, and a byte below 0x80 is always
** its own character. */

/* Bytes of the character at s, given at most `max` bytes: 0 when max is
   0, otherwise at least 1. */
size_t	mb_len(const char *s, size_t max)
{
	mbstate_t	st;
	size_t		r;

	if (max == 0)
		return (0);
	if ((unsigned char)*s < 0x80 || MB_CUR_MAX == 1)
		return (1);
	ft_memset(&st, 0, sizeof(st));
	r = mbrtowc(NULL, s, max, &st);
	if (r == (size_t)-1 || r == (size_t)-2 || r == 0)
		return (1);
	return (r);
}

/* The character at the head of a NUL-terminated string. */
size_t	mb_len0(const char *s)
{
	size_t	n;

	n = 0;
	while (n < (size_t)MB_CUR_MAX && s[n])
		n++;
	return (mb_len(s, n));
}

/* Characters in s[0..n). */
size_t	mb_count(const char *s, size_t n)
{
	size_t	i;
	size_t	c;

	i = 0;
	c = 0;
	while (i < n)
	{
		i += mb_len(s + i, n - i);
		c++;
	}
	return (c);
}

/* Byte offset of character `nth` (0-based) in s[0..n); n when the string
   has fewer characters than that. */
size_t	mb_skip(const char *s, size_t n, size_t nth)
{
	size_t	i;

	i = 0;
	while (i < n && nth > 0)
	{
		i += mb_len(s + i, n - i);
		nth--;
	}
	if (i > n)
		return (n);
	return (i);
}

/* The character at s (n bytes) case-converted under op -- '^' upper, ','
   lower, '~' toggle -- re-encoded into out (at least MB_LEN_MAX bytes).
   Returns the bytes written, or 0 when the character does not decode or
   the locale cannot encode the result, and the caller keeps the original
   bytes. */
size_t	mb_conv(const char *s, size_t n, char op, char *out)
{
	mbstate_t	st;
	wchar_t		wc;
	size_t		r;

	ft_memset(&st, 0, sizeof(st));
	if (mbrtowc(&wc, s, n, &st) != n)
		return (0);
	if (op == '^' || (op == '~' && !iswupper((wint_t)wc)))
		wc = (wchar_t)towupper((wint_t)wc);
	else
		wc = (wchar_t)towlower((wint_t)wc);
	ft_memset(&st, 0, sizeof(st));
	r = wcrtomb(out, wc, &st);
	if (r == (size_t)-1)
		return (0);
	return (r);
}
