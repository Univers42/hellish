/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   helper2.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/18 23:05:17 by marvin            #+#    #+#             */
/*   Updated: 2026/01/18 23:05:17 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "lexer.h"
#include "sys.h"

/* Shell whitespace is only space and tab -- newline is a token separator,
   not ignorable whitespace, so we must NOT include it here. */
bool	is_space(char c)
{
	return (c == ' ' || c == '\t');
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
	while (s[i] && ft_isdigit((unsigned char)s[i]) && i < 3)
		i++;
	if (i > 0 && i <= 2 && (s[i] == '<' || s[i] == '>'))
		return (true);
	return (false);
}

/* Any character that the shell treats as a metacharacter or whitespace. The
   SPECIAL_CHARS macro expands to the set of operator-starting chars; together
   with is_space these cover every character that cannot appear unquoted inside
   a plain word token. */
bool	is_special_char(char c)
{
	char	*specials;

	specials = SPECIAL_CHARS;
	if (ft_strchr(specials, c) || is_space(c))
		return (true);
	return (false);
}

/* Check if the current position ends a bare-word token. A fd-redirect start
   (e.g. "2>") is a boundary too -- otherwise `echo2>file` would tokenise as
   one word instead of `echo` + `2>` + `file`. */
bool	is_word_boundary(const char *s)
{
	if (is_special_char(*s))
		return (true);
	if (is_fd_redirect_start(s))
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
