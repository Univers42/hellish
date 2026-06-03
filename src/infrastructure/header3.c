/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   header3.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/03 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/03 13:00:17 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "header_private.h"

/* Print `n` blank spaces. */
void	emit_spaces(int n)
{
	while (n-- > 0)
		ft_eprintf(" ");
}

/* The line at index `i`, or NULL once the array runs out (so the shorter
   column simply pads with blank cells). */
const char	*panel_nth(const char **a, int i)
{
	int	n;

	n = 0;
	if (!a)
		return (NULL);
	while (a[n])
		n++;
	if (i < n)
		return (a[i]);
	return (NULL);
}

/* Emit one column cell, reading its leading marker for style: a rule, a
   coloured logo line, an accented heading, or plain muted text. */
static void	emit_marked(const char *line, int width, int align,
		const char *logo_color)
{
	if (!line)
		return (emit_cell("", width, align, C_TAG));
	if (line[0] == '\001')
		return (emit_rule_cell(width));
	if (line[0] == '\002')
		return (emit_cell(line + 1, width, align, C_HEAD));
	if (line[0] == '\003')
		return (emit_cell(line + 1, width, 1, logo_color));
	emit_cell(line, width, align, C_TAG);
}

/* One body row: side border, centred left cell, divider, left-aligned right
   cell, side border. */
void	body_row(const t_panel *p, const char *l, const char *r, t_cols c)
{
	t_glyphs	g;

	g = header_glyphs();
	ft_eprintf(C_FRAME "%s" C_RST " ", g.v);
	emit_marked(l, c.lw, 1, p->logo_color);
	ft_eprintf(" " C_FRAME "%s" C_RST " ", g.v);
	emit_marked(r, c.rw, 0, p->logo_color);
	ft_eprintf(" " C_FRAME "%s" C_RST "\n", g.v);
}

/* The closing rounded border, spanning `cols` columns. */
void	bottom_rule(int cols)
{
	t_glyphs	g;
	int			i;

	g = header_glyphs();
	ft_eprintf(C_FRAME "%s", g.bl);
	i = 2;
	while (i < cols)
	{
		ft_eprintf("%s", g.h);
		i++;
	}
	ft_eprintf("%s" C_RST "\n", g.br);
}
