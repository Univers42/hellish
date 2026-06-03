/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   prompt_anim3.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/03 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/03 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "prompt_styles.h"

/* How many lines sit above the input arrow (one per newline in the prompt). */
static int	count_nl(const char *s)
{
	int	n;

	n = 0;
	while (*s)
	{
		if (*s++ == '\n')
			n++;
	}
	return (n);
}

/* Move the cursor up `n` rows (ESC [ n A). */
static void	cursor_up(int n)
{
	char	*s;

	s = ft_itoa(n);
	if (!s)
		return ;
	fputs("\033[", rl_outstream);
	fputs(s, rl_outstream);
	fputs("A", rl_outstream);
	free(s);
}

/* Emit one prompt char to the terminal: drop the \001/\002 width markers and
   turn each newline into "down a row, return, clear" so every sprite row is
   wiped before it is repainted. */
static void	emit_top_char(char c)
{
	if (c == '\001' || c == '\002')
		return ;
	if (c == '\n')
	{
		fputs("\n\r\033[2K", rl_outstream);
		return ;
	}
	fputc((unsigned char)c, rl_outstream);
}

/* Repaint the whole sprite block in place: hardware-save the cursor, step up
   onto the first sprite row, clear+print every row above the input line, then
   restore the cursor exactly where readline left it. Works for any height. */
void	redraw_top(t_string *r)
{
	char	*s;
	char	*last;
	size_t	i;

	s = (char *)r->ctx;
	last = ft_strrchr(s, '\n');
	if (!last)
		return ;
	fputs("\0337", rl_outstream);
	cursor_up(count_nl(s));
	fputs("\r\033[2K", rl_outstream);
	i = 0;
	while (s + i < last)
		emit_top_char(s[i++]);
	fputs("\0338", rl_outstream);
	fflush(rl_outstream);
}
