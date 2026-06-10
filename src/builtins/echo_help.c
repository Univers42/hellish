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

/* Parse an octal (\0NNN) or hex (\xNN) numeric escape in echo -e. The `str`
   pointer is advanced past the digits that were consumed so the caller's
   loop lands on the next character to process. Only the first 2 hex or 3
   octal digits are taken; the rest are printed literally. The decoded byte
   is appended to `out` (echo gathers everything into one buffer and does a
   single write at the end). */
void	parse_numeric_escape(t_vec *out, char **str)
{
	int				base;
	unsigned char	c;
	char			*end;
	int				val;

	base = 10;
	if (**str == '0')
		base = 8;
	else if (**str == 'x')
		base = 16;
	else
		return ;
	(*str)++;
	val = ft_strto_int(*str, &end, base);
	if (end && end != *str)
		*str = end;
	c = (unsigned char)val;
	vec_push_char(out, (char)c);
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
