/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   header4.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/03 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/03 13:00:19 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "header_private.h"

/* Length in bytes of an ANSI CSI escape (ESC '[' … final) at `s`, else 0. Used
   so width/centring ignore embedded colours. */
int	skip_ansi(const char *s)
{
	int	i;

	if (s[0] != 27 || s[1] != '[')
		return (0);
	i = 2;
	while (s[i] && (s[i] < '@' || s[i] > '~'))
		i++;
	if (s[i])
		i++;
	return (i);
}

/* The number of body rows: the taller of the two columns. */
static int	panel_rows(const t_panel *p)
{
	int	ln;
	int	rn;

	ln = 0;
	rn = 0;
	while (p->left && p->left[ln])
		ln++;
	while (p->right && p->right[rn])
		rn++;
	if (ln > rn)
		return (ln);
	return (rn);
}

/* Pick the left column width from its content, then clamp so the right column
   keeps a sane minimum even on narrow terminals. */
static t_cols	panel_layout(const t_panel *p, int cols)
{
	t_cols	c;

	c.lw = panel_max_width(p->left) + 2;
	if (c.lw > cols - 19)
		c.lw = cols - 19;
	if (c.lw < 8)
		c.lw = 8;
	c.rw = cols - c.lw - 7;
	if (c.rw < 1)
		c.rw = 1;
	return (c);
}

/* Render the whole panel: a blank line, the title-bearing top border, the two
   columns row by row, then the closing border. It spans the full terminal
   width and truncates its cells to fit narrow windows. */
void	render_panel(const t_panel *p)
{
	t_cols	c;
	int		cols;
	int		rows;
	int		i;

	cols = header_cols();
	c = panel_layout(p, cols);
	rows = panel_rows(p);
	ft_eprintf("\n");
	title_rule(p->title, cols);
	i = -1;
	while (++i < rows)
		body_row(p, panel_nth(p->left, i), panel_nth(p->right, i), c);
	bottom_rule(cols);
}
