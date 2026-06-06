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
   octal digits are taken; the rest are printed literally. */
void	parse_numeric_escape(char **str)
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
	ft_putchar_fd((char)c, 1);
}

/* Emit the character that corresponds to a single-letter escape (\n, \t …).
   Returns 1 if the escape was recognised and printed, 0 if not — the caller
   then prints a literal backslash followed by the character. */
static int	backslash_writer(char *s)
{
	if (*s == 'n')
		ft_putchar_fd('\n', 1);
	else if (*s == 't')
		ft_putchar_fd('\t', 1);
	else if (*s == 'a')
		ft_putchar_fd('\a', 1);
	else if (*s == 'b')
		ft_putchar_fd('\b', 1);
	else if (*s == 'f')
		ft_putchar_fd('\f', 1);
	else if (*s == 'r')
		ft_putchar_fd('\r', 1);
	else if (*s == 'v')
		ft_putchar_fd('\v', 1);
	else if (*s == '\\')
		ft_putchar_fd('\\', 1);
	else if (*s == 'e')
		ft_putstr_fd("\033", 1);
	else
		return (0);
	return (1);
}

/* Interpret the string `s` with escape processing (echo -e). Walk character
   by character; on a backslash peek one ahead. '\c' returns 1 immediately,
   telling the caller to suppress the trailing newline and stop all output.
   Everything else is routed through backslash_writer or parse_numeric_escape;
   unrecognised escapes print the backslash and the following character. */
int	e_parser(char *s)
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
				parse_numeric_escape(&s);
				continue ;
			}
			else if (!backslash_writer(s))
			{
				ft_putchar_fd('\\', 1);
				ft_putchar_fd(*s, 1);
			}
			s++;
		}
		else
			ft_putchar_fd(*s++, 1);
	}
	return (0);
}
