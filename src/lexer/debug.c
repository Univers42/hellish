/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   debug.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/12 01:54:58 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/19 20:21:57 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "lexer.h"
#include "libft.h"
#include <stdio.h>
#include <stdlib.h>

char	*tt_to_str(t_tt tt)
{
	const char	**names;

	names = get_tt_names();
	if ((unsigned)tt < 256)
		return ((char *)names[tt]);
	return ("TT_INVALID");
}

const char	*token_color(t_tt tt)
{
	t_hash			*map;
	const char		*c;
	const char		*name;

	map = get_color_map();
	if (!map)
		return (ASCII_BLUE);
	name = tt_to_str(tt);
	c = (const char *)hash_get(map, name);
	if (c)
		return (c);
	return (ASCII_BLUE);
}

size_t	visible_lexeme_len(t_token *t)
{
	size_t			i;
	size_t			len;
	unsigned char	c;

	i = -1;
	len = 0;
	while (++i < (size_t)t->len)
	{
		c = (unsigned char)t->start[i];
		if (c == '\n' || c == '\t')
			len += 2;
		else
			len += 1;
	}
	return (len);
}

void	print_visible_lexeme_noquotes(t_token *t)
{
	size_t			i;
	unsigned char	c;

	i = -1;
	while (++i < (size_t)t->len)
	{
		c = (unsigned char)t->start[i];
		if (c == '\n')
			ft_printf("\\n");
		else if (c == '\t')
			ft_printf("\\t");
		else
			ft_printf("%c", c);
	}
}
