/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   helper2.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/18 23:05:17 by marvin            #+#    #+#             */
/*   Updated: 2026/07/21 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "lexer.h"
#include "sys.h"

#define CL_SPEC		1
#define CL_SPACE	2
#define CL_DIGIT	4

/* Character-class table replacing the old per-call ft_strchr over
   SPECIAL_CHARS — these predicates run over a million times on a 50k-line
   parse and the strchr walk dominated them. Bit 1 = shell metacharacter
   (the SPECIAL_CHARS set, PLUS index 0: ft_strchr matches the terminator,
   so NUL has always counted as special — end of input ends a word).
   Bit 2 = blank (space/tab), bit 4 = ASCII digit. Row comments mark the
   non-zero islands. */
static const unsigned char	g_cl[256] = {
	1, 0, 0, 0, 0, 0, 0, 0, 0, 2, 1, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	2, 0, 1, 0, 1, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0,
	4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 0, 1, 1, 0, 1, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

/* Shell whitespace is only space and tab -- newline is a token separator,
   not ignorable whitespace, so we must NOT include it here. */
bool	is_space(char c)
{
	return ((g_cl[(unsigned char)c] & CL_SPACE) != 0);
}

/* Returns true when the string starts with 1-2 digits immediately followed
   by a redirect char. We cap at 2 digits -- POSIX only mandates fd < 10 but
   bash handles up to 9 too; no sane shell supports fd 100 at lex time. This
   check is also called from is_word_boundary so `2>&1` never merges with a
   preceding word. */
static bool	is_fd_redirect_start(const char *s)
{
	int	i;

	i = 0;
	while ((g_cl[(unsigned char)s[i]] & CL_DIGIT) && i < 3)
		i++;
	if (i > 0 && i <= 2 && (s[i] == '<' || s[i] == '>'))
		return (true);
	return (false);
}

/* Any character that the shell treats as a metacharacter or whitespace:
   one table load instead of an ft_strchr walk per query. */
bool	is_special_char(char c)
{
	return ((g_cl[(unsigned char)c] & (CL_SPEC | CL_SPACE)) != 0);
}

/* Check if the current position ends a bare-word token. A fd-redirect start
   (e.g. "2>") is a boundary too -- otherwise `echo2>file` would tokenise as
   one word instead of `echo` + `2>` + `file`. Only a digit can start a
   fd-redirect, so the scan is gated on the digit bit. */
bool	is_word_boundary(const char *s)
{
	unsigned char	cl;

	cl = g_cl[(unsigned char)*s];
	if (cl & (CL_SPEC | CL_SPACE))
		return (true);
	if ((cl & CL_DIGIT) && is_fd_redirect_start(s))
		return (true);
	return (false);
}

/* Count decimal digits in v (minimum 1). Used by the debug table renderer
   to size the "len" column without calling snprintf. */
size_t	num_digits(size_t v)
{
	size_t	d;

	d = 1;
	while (v >= 10)
	{
		v /= 10;
		d++;
	}
	return (d);
}
