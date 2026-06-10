/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   echo_help.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 23:30:11 by marvin            #+#    #+#             */
/*   Updated: 2026/01/09 23:30:11 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"

/* Value of a hex digit, or -1 when `c` is not one. */
static int	xdigit_value(char c)
{
	if (c >= '0' && c <= '9')
		return (c - '0');
	if (c >= 'a' && c <= 'f')
		return (c - 'a' + 10);
	if (c >= 'A' && c <= 'F')
		return (c - 'A' + 10);
	return (-1);
}

/* \0NNN / \xHH numeric escapes for echo -e, with bash's digit caps: \0
   consumes AT MOST three octal digits (zero digits still emits a NUL
   byte: `\0z` -> NUL 'z'); \x consumes one or two hex digits, and with
   no digit at all bash keeps the \x as literal text (`\xg` -> \ x g).
   Values above 0xFF wrap to a byte (\0777 -> 0xFF), like bash.  The old
   code fed ft_strto_int with no digit limit, so `\x41d` parsed as 0x41d
   and `\0101e` swallowed nothing it should.  *str arrives on the
   '0'/'x' marker and leaves on the first unconsumed character. */
static void	push_octal(t_vec *out, char **str)
{
	int	val;
	int	n;

	val = 0;
	n = 0;
	while (n < 3 && **str >= '0' && **str <= '7')
	{
		val = val * 8 + (*(*str)++ - '0');
		n++;
	}
	vec_push_char(out, (char)val);
}

void	parse_numeric_escape(t_vec *out, char **str)
{
	int	val;
	int	n;

	if (**str == '0')
	{
		(*str)++;
		push_octal(out, str);
		return ;
	}
	(*str)++;
	val = 0;
	n = 0;
	while (n < 2 && xdigit_value(**str) >= 0)
	{
		val = val * 16 + xdigit_value(*(*str)++);
		n++;
	}
	if (n == 0)
	{
		vec_push_str(out, "\\x");
		return ;
	}
	vec_push_char(out, (char)val);
}

/* Append the character that corresponds to a single-letter escape (\n, \t …)
   to `out`.  Returns 1 if the escape was recognised, 0 if not — the caller
   then appends a literal backslash followed by the character. */
static int	backslash_writer(t_vec *out, char *s)
{
	if (*s == 'n')
		vec_push_char(out, '\n');
	else if (*s == 't')
		vec_push_char(out, '\t');
	else if (*s == 'a')
		vec_push_char(out, '\a');
	else if (*s == 'b')
		vec_push_char(out, '\b');
	else if (*s == 'f')
		vec_push_char(out, '\f');
	else if (*s == 'r')
		vec_push_char(out, '\r');
	else if (*s == 'v')
		vec_push_char(out, '\v');
	else if (*s == '\\')
		vec_push_char(out, '\\');
	else if (*s == 'e')
		vec_push_char(out, '\033');
	else
		return (0);
	return (1);
}

/* Interpret the string `s` with escape processing (echo -e), appending the
   decoded bytes to `out`. Walk character by character; on a backslash peek
   one ahead. '\c' returns 1 immediately, telling the caller to suppress the
   trailing newline and stop all output (what was buffered so far is still
   written). Everything else is routed through backslash_writer or
   parse_numeric_escape; unrecognised escapes keep the backslash and the
   following character. */
int	e_parser(t_vec *out, char *s)
{
	while (*s)
	{
		if (*s == '\\' && s[1])
		{
			s++;
			if (*s == 'c')
				return (1);
			else if (*s == '0' || *s == 'x')
			{
				parse_numeric_escape(out, &s);
				continue ;
			}
			else if (!backslash_writer(out, s))
			{
				vec_push_char(out, '\\');
				vec_push_char(out, *s);
			}
			s++;
		}
		else
			vec_push_char(out, *s++);
	}
	return (0);
}
