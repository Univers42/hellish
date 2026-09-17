/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   expand_param_trimlit.c                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/17 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/09/17 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"
#include "mbchar.h"
#include <stdlib.h>

/* The literal text of a trim pattern, or NULL when a character in it is
   still a live glob character. Quoted text arrives backslash-escaped
   (word_to_pattern), so `\*` is a star and a bare one is not; `(` counts
   as live because it opens every extglob group (patsub_match_len, which
   makes the same call). NULL too in a multibyte locale that is not UTF-8,
   where a byte comparison cannot tell where characters start. */
static char	*trim_unescape(const char *pat, size_t *n)
{
	char	*lit;
	size_t	i;

	if (MB_CUR_MAX > 1 && !mb_is_utf8())
		return (NULL);
	lit = xmalloc(ft_strlen(pat) + 1);
	if (!lit)
		return (NULL);
	*n = 0;
	i = 0;
	while (pat[i])
	{
		if (pat[i] == '\\' && pat[i + 1])
			i++;
		else if (ft_strchr("*?[(", pat[i]))
			return (xfree(lit), NULL);
		lit[(*n)++] = pat[i++];
	}
	return (lit);
}

/* Can a match start or end at byte `at` of val? Always in a single-byte
   locale; in UTF-8 unless `at` is a continuation byte. */
static bool	trim_boundary(const char *val, size_t at)
{
	if (MB_CUR_MAX == 1)
		return (true);
	return (((unsigned char)val[at] & 0xC0) != 0x80);
}

/* ${v%lit} ${v%%lit} ${v#lit} ${v##lit} when the pattern globs nothing.
**
** A literal pattern can match in exactly one place, so shortest and
** longest are the same and one comparison answers it. The matcher walks
** every position instead, running a full match at each, and the idiom
** that peels one character off a string --
**
**     c=${s%"${s#?}"}
**
** -- is a literal pattern as long as the string itself: 105 ms against
** bash's 24 ms to walk 300 characters. NULL means "not for this path":
** a globbing pattern, a multibyte locale that is not UTF-8, or a match
** that would split a character; the caller then runs the matcher. */
char	*trim_literal(const char *val, const char *pat, bool suffix)
{
	char	*lit;
	size_t	n;
	size_t	vlen;
	size_t	start;
	size_t	at;

	lit = trim_unescape(pat, &n);
	if (!lit)
		return (NULL);
	vlen = ft_strlen(val);
	start = 0;
	if (suffix && n <= vlen)
		start = vlen - n;
	if (n > vlen || ft_memcmp(val + start, lit, n) != 0)
		return (xfree(lit), ft_strdup(val));
	xfree(lit);
	at = n;
	if (suffix)
		at = start;
	if (!trim_boundary(val, at))
		return (NULL);
	if (suffix)
		return (ft_strndup(val, at));
	return (ft_strdup(val + n));
}

/* The general case: the matcher, for the operator at `op`. */
char	*trim_by_matcher(const char *val, const char *pat, const char *op)
{
	if (op[0] == '%' && op[1] == '%')
		return (trim_suffix_longest(val, pat));
	if (op[0] == '%')
		return (trim_suffix_shortest(val, pat));
	if (op[0] == '#' && op[1] == '#')
		return (trim_prefix_longest(val, pat));
	if (op[0] == '#')
		return (trim_prefix_shortest(val, pat));
	return (ft_strdup(val));
}
