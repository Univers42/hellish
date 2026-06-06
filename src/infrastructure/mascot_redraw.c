/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   mascot_redraw.c                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/03 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/03 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "prompt_private.h"

/* Prompt repainting support used by the readline idle hook (mascot_hook).
   The prompt is two rows: the top info bar and the bottom arrow. When the hook
   fires during typing we need to repaint the top row in place without moving
   the cursor away from the typed text. The trick is VT100 cursor save/restore
   (\0337 / \0338): save position, jump up and repaint, restore. */

/* Lines above the input arrow (one per newline in the rendered prompt). */
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

/* Move the cursor up n rows (ESC [ n A). */
static void	cursor_up(int n)
{
	char	*s;

	s = ft_itoa(n);
	if (!s)
		return ;
	fputs("\033[", rl_outstream);
	fputs(s, rl_outstream);
	fputs("A", rl_outstream);
	xfree(s);
}

/* Emit one prompt byte: drop the \001/\002 width markers, and turn newlines
   into "down a row, return, clear" so each repainted row is wiped first. */
static void	emit_char(char c)
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

/* How many rows up the prompt's first row is from the cursor: the prompt rows
   above the input, plus however far the input itself has wrapped (the bottom
   "╰─❯ " arrow is 4 columns + rl_point typed characters). */
static int	rows_above(const char *s)
{
	int	cols;

	cols = get_cols();
	if (cols < 2)
		cols = 80;
	return (count_nl(s) + (4 + rl_point) / cols);
}

/* Repaint the mascot line in place without disturbing what is being typed:
   hardware-save the cursor, step up onto the prompt's first row, clear+print
   every row above the input, then restore the cursor exactly. */
void	redraw_mascot(t_string *r)
{
	char	*s;
	char	*last;
	size_t	i;

	s = (char *)r->ctx;
	last = ft_strrchr(s, '\n');
	if (!last)
		return ;
	fputs("\0337", rl_outstream);
	cursor_up(rows_above(s));
	fputs("\r\033[2K", rl_outstream);
	i = 0;
	while (s + i < last)
		emit_char(s[i++]);
	fputs("\0338", rl_outstream);
	fflush(rl_outstream);
}
