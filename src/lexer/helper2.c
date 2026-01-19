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

bool	is_special_char(char c)
{
	char	*specials;

	specials = ";$'\"<>|&()\n";
	if (ft_strchr(specials, c) || is_space(c))
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
