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

void parse_numeric_escape(char **str)
{
	int base;
	unsigned char c;
	char *end;
	int val;

	base = 10;
	if (**str == '0')
		base = 8;
	else if (**str == 'x')
		base = 16;
	else
		return;
	(*str)++;
	val = ft_strto_int(*str, &end, base);
	if (end && end != *str)
		*str = end;
	c = (unsigned char)val;
	ft_putchar_fd((char)c, 1);
}

static int backslash_writer(char *s)
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

int e_parser(char *s)
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
				continue;
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
