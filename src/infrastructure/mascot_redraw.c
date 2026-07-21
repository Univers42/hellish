/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   mascot_redraw.c                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/03 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/07/21 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "prompt_private.h"

/* In-place repaint of the prompt rows above the input, used by the
   animation idle hook. The first version streamed the frame through
   unbuffered fputc on stderr — dozens of write() syscalls per tick, so
   the terminal rendered intermediate states and the caret visibly
   ping-ponged between the info row and the input row (flicker). Now the
   ENTIRE frame is composed in memory and delivered in ONE write(),
   wrapped in cursor-hide (DECTCEM) and synchronized-output (DEC 2026,
   ignored where unsupported): the caret never appears away from the
   input line and the repaint lands as a single atomic update. */

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

/* Append "move cursor up n rows" (ESC [ n A) to the frame buffer. */
static void	rd_up(t_string *f, int n)
{
	char	*s;

	s = ft_itoa(n);
	if (!s)
		return ;
	vec_push_str(f, "\033[");
	vec_push_str(f, s);
	vec_push_str(f, "A");
	xfree(s);
}

/* Append one prompt byte: drop the \001/\002 width markers, and turn
   newlines into "down a row, return, clear" so each repainted row is
   wiped before its new content lands. */
static void	rd_char(t_string *f, char c)
{
	if (c == '\001' || c == '\002')
		return ;
	if (c == '\n')
	{
		vec_push_str(f, "\n\r\033[2K");
		return ;
	}
	vec_push_char(f, c);
}

/* How many rows up the prompt's first row is from the cursor: the prompt
   rows above the input, plus however far the input itself has wrapped.
   The input row starts after the prompt's last-row visible width, which
   ps1_animated measured into the cells (4 was the old built-in arrow's
   hardcoded width, kept as the fallback). */
static int	rows_above(const char *s)
{
	int	cols;
	int	lastw;

	cols = get_cols();
	if (cols < 2)
		cols = 80;
	lastw = 4;
	if (anim_cells()->count > 0)
		lastw = anim_cells()->last_w;
	return (count_nl(s) + (lastw + rl_point) / cols);
}

/* Compose and emit one repaint frame: sync+hide, save cursor, jump up,
   clear+rewrite every row above the input, restore, show, unsync — all
   in a single write so the terminal treats it as one update. */
void	redraw_mascot(t_string *r)
{
	t_string	f;
	char		*s;
	char		*last;
	size_t		i;

	s = (char *)r->ctx;
	last = ft_strrchr(s, '\n');
	if (!last)
		return ;
	vec_init(&f);
	f.elem_size = 1;
	vec_push_str(&f, "\033[?2026h\033[?25l\0337");
	rd_up(&f, rows_above(s));
	vec_push_str(&f, "\r\033[2K");
	i = 0;
	while (s + i < last)
		rd_char(&f, s[i++]);
	vec_push_str(&f, "\0338\033[?25h\033[?2026l");
	if (write(fileno(rl_outstream), f.ctx, f.len) < 0)
		f.len = 0;
	xfree(f.ctx);
}
