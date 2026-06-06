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

/* Convert a token type to its human-readable name for debug output. The
   names table covers values 0-255; anything outside that range (should never
   happen) returns "TT_INVALID" so we never dereference out-of-bounds. */
char	*tt_to_str(t_tt tt)
{
	const char	**names;

	names = get_tt_names();
	if ((unsigned)tt < 256)
		return ((char *)names[tt]);
	return ("TT_INVALID");
}

/* Look up the ANSI colour assigned to this token type. Falls back to blue
   when the colour map is unavailable or the type has no entry -- blue is
   visually neutral and never clashes with the magenta table borders. */
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

/* Calculate the number of columns a lexeme will occupy on screen, expanding
   `\n` and `\t` to two-char escape sequences. This is used to pad columns
   in the debug table to the same width without calling wcswidth. */
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

/* Print the token's raw bytes to stdout, substituting `\n` / `\t` with
   their two-character escape representation. We never add surrounding quotes
   so the displayed text is exactly what was in the input, just readable. */
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
