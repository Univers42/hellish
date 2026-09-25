/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   helper2.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/18 23:05:17 by marvin            #+#    #+#             */
/*   Updated: 2026/09/25 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "lexer.h"
#include "sys.h"

#define CL_SPEC		1
#define CL_SPACE	2

/* Character-class table replacing the old per-call ft_strchr over
   SPECIAL_CHARS — these predicates run over a million times on a 50k-line
   parse and the strchr walk dominated them. Bit 1 = shell metacharacter
   (the SPECIAL_CHARS set, PLUS index 0: ft_strchr matches the terminator,
   so NUL has always counted as special — end of input ends a word).
   Bit 2 = blank (space/tab). Row comments mark the non-zero islands. */
static const unsigned char	g_cl[256] = {
	1, 0, 0, 0, 0, 0, 0, 0, 0, 2, 1, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	2, 0, 1, 0, 1, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 0,
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

/* Any character that the shell treats as a metacharacter or whitespace:
   one table load instead of an ft_strchr walk per query. */
bool	is_special_char(char c)
{
	return ((g_cl[(unsigned char)c] & (CL_SPEC | CL_SPACE)) != 0);
}

/* Check if the current position ends a bare-word token: a metacharacter
   or a blank. Digits before a `<` or `>` do not end one -- an fd number is
   a whole token or nothing (io_number_at, helper4.c), so `echo2>file` is
   the word `echo2` redirected to file, as in bash, not `echo` + `2>`. */
bool	is_word_boundary(const char *s)
{
	return ((g_cl[(unsigned char)*s] & (CL_SPEC | CL_SPACE)) != 0);
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
