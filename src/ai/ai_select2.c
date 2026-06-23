/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ai_select2.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/22 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/22 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ai_select.h"
#include <unistd.h>

/* Draw one option row, highlighting the selected one. */
static void	draw_row(t_aisel *s, int i)
{
	ft_eprintf("\r\033[2K");
	if (i == s->sel)
		ft_eprintf("\033[1;38;5;209m > %s\033[0m\n", s->all[s->idx[i]]);
	else
		ft_eprintf("   \033[38;5;250m%s\033[0m\n", s->all[s->idx[i]]);
}

/* Erase rows left over from a taller previous frame, then step back up. */
static void	clear_tail(t_aisel *s, int drawn)
{
	int	extra;

	extra = s->prev - drawn;
	if (extra <= 0)
		return ;
	while (extra-- > 0)
		ft_eprintf("\r\033[2K\n");
	ft_eprintf("\033[%dA", s->prev - drawn);
}

/* Repaint the filter line + all matched rows in place. */
static void	draw(t_aisel *s)
{
	int	i;
	int	drawn;

	if (s->prev > 0)
		ft_eprintf("\033[%dA", s->prev);
	ft_eprintf("\r\033[2K  search: %s\n", s->filt);
	drawn = 1;
	i = 0;
	while (i < s->m)
	{
		draw_row(s, i);
		drawn++;
		i++;
	}
	clear_tail(s, drawn);
	s->prev = drawn;
}

/* Event loop: read a key burst, apply it, repaint. Returns 1 or -1. */
static int	sel_loop(t_aisel *s)
{
	unsigned char	b[8];
	int				k;
	int				r;

	ai_refilter(s);
	draw(s);
	k = 0;
	while (k == 0)
	{
		r = (int)read(STDIN_FILENO, b, sizeof(b));
		k = ai_on_key(s, b, r);
		if (k == 0)
			draw(s);
	}
	return (k);
}

/* Fuzzy picker. -1 on cancel / no TTY; else the chosen index into names. */
int	ai_select(char **names, int n, const char *title)
{
	struct termios	saved;
	t_aisel			s;
	int				k;
	int				res;

	if (n <= 0 || !isatty(STDIN_FILENO) || !ai_raw_on(&saved))
		return (-1);
	ft_bzero(&s, sizeof(s));
	s.all = names;
	s.n = n;
	s.idx = xmalloc(sizeof(int) * n);
	ft_eprintf("%s (type to filter, up/down, Enter, Esc)\n", title);
	k = sel_loop(&s);
	tcsetattr(STDIN_FILENO, TCSANOW, &saved);
	ft_eprintf("\n");
	res = -1;
	if (k == 1 && s.m > 0)
		res = s.idx[s.sel];
	xfree(s.idx);
	return (res);
}
