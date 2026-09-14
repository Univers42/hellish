/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   env_quote2.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/14 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/09/14 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "env.h"
#include "mbchar.h"

/* How declare -p spells a value.

   bash: double quotes unless the value has a character that cannot be
   read back from them -- a newline, a tab, an escape, anything below a
   space, the DEL, and any byte that is not a character in this locale --
   and then the ANSI-C form, $'...', with \n \t \E and octal for the
   rest. In a UTF-8 locale a valid multibyte character is printed as
   itself either way; in the C locale every byte above 0x7f is octal.
   hellish always used double quotes, so `declare -p PROMPT_COMMAND` after
   bash-preexec, whose first element has a newline in it, printed the
   newline itself, and nothing that read the output back got the same
   variable. This is the one place either form is produced; arr_format,
   assoc_format and declare -p all come here. */

/* Bytes of the valid multibyte character at s, or 0 when the byte is not
   one: in the C locale that is every byte above 0x7f. */
static size_t	mb_valid(const char *s, size_t max)
{
	size_t	n;

	if ((unsigned char)*s < 0x80)
		return (1);
	n = mb_len(s, max);
	if (n < 2)
		return (0);
	return (n);
}

static bool	needs_ansic(const char *s, int len)
{
	size_t	i;
	size_t	n;

	i = 0;
	while (i < (size_t)len)
	{
		n = mb_valid(s + i, (size_t)len - i);
		if (n == 0 || (unsigned char)s[i] < 32 || s[i] == 127)
			return (true);
		i += n;
	}
	return (false);
}

/* One byte that is not a character of its own: \a..\r by name, \E for
   the escape, a backslash before \ and ', octal for everything else. */
static void	push_ansic_char(t_string *out, unsigned char c)
{
	const char	*esc;

	esc = "\\a\\b\\t\\n\\v\\f\\r";
	if (c >= 7 && c <= 13)
	{
		vec_push_char(out, esc[(c - 7) * 2]);
		vec_push_char(out, esc[(c - 7) * 2 + 1]);
	}
	else if (c == 27)
		vec_push_str(out, "\\E");
	else if (c >= 32 && c < 127)
	{
		if (c == '\\' || c == '\'')
			vec_push_char(out, '\\');
		vec_push_char(out, (char)c);
	}
	else
	{
		vec_push_char(out, '\\');
		vec_push_char(out, '0' + (c >> 6));
		vec_push_char(out, '0' + ((c >> 3) & 7));
		vec_push_char(out, '0' + (c & 7));
	}
}

/* The body of the $'...' form: a valid multibyte character as itself,
   every other byte through push_ansic_char. */
static void	push_ansic(t_string *out, const char *s, int len)
{
	size_t	i;
	size_t	n;

	i = 0;
	while (i < (size_t)len)
	{
		n = mb_valid(s + i, (size_t)len - i);
		if (n > 1)
			vec_push_nstr(out, s + i, n);
		else
			push_ansic_char(out, (unsigned char)s[i]);
		i += n + (n == 0);
	}
}

/* Push `len` bytes of s as one shell word that reads back as s. */
void	vec_push_value(t_string *out, const char *s, int len)
{
	if (!s || !needs_ansic(s, len))
	{
		vec_push_char(out, '"');
		if (s)
			vec_push_dquoted(out, s, len);
		vec_push_char(out, '"');
		return ;
	}
	vec_push_str(out, "$'");
	push_ansic(out, s, len);
	vec_push_char(out, '\'');
}
