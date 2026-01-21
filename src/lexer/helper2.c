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

bool	is_space(char c)
{
	return (c == ' ' || c == '\t');
}

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

bool	is_special_char(char c)
{
	char	*specials;

	specials = ";$'\"<>|&()\n";
	if (ft_strchr(specials, c) || is_space(c))
		return (true);
	return (false);
}

/* Check if current position in string should end a word token */
bool	is_word_boundary(const char *s)
{
	if (is_special_char(*s))
		return (true);
	if (is_fd_redirect_start(s))
		return (true);
	return (false);
}

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
