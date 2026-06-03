/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   header2.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/03 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/03 13:00:17 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "header_private.h"

/* Print exactly `width` columns of horizontal rule, in the frame colour. */
void	emit_rule_cell(int width)
{
	t_glyphs	g;
	int			i;

	g = header_glyphs();
	ft_eprintf(C_FRAME);
	i = 0;
	while (i++ < width)
		ft_eprintf("%s", g.h);
	ft_eprintf(C_RST);
}

/* Fit `text` into `buf` at most `width` columns, adding an ellipsis if cut. */
static void	cell_fit(char *buf, size_t n, const char *text, int width)
{
	t_glyphs	g;

	g = header_glyphs();
	if (header_width(text) > width)
	{
		header_clip(buf, n, text, width - 1);
		ft_strlcat(buf, g.ell, n);
	}
	else
		ft_strlcpy(buf, text, n);
}

/* Print `text` in `color`, padded or truncated to exactly `width` columns. */
void	emit_cell(const char *text, int width, int align, const char *color)
{
	char	buf[1024];
	int		pad;
	int		lp;

	if (!text)
		text = "";
	cell_fit(buf, sizeof(buf), text, width);
	pad = width - header_width(buf);
	if (pad < 0)
		pad = 0;
	lp = 0;
	if (align == 1)
		lp = pad / 2;
	ft_eprintf("%s", color);
	emit_spaces(lp);
	ft_eprintf("%s", buf);
	emit_spaces(pad - lp);
	ft_eprintf(C_RST);
}

/* The widest line (display columns) in a NULL-terminated, marked array. */
int	panel_max_width(const char **lines)
{
	int			best;
	int			w;
	const char	*s;

	best = 0;
	while (lines && *lines)
	{
		s = *lines;
		if (*s == '\001')
			w = 0;
		else if (*s == '\002' || *s == '\003')
			w = header_width(s + 1);
		else
			w = header_width(s);
		if (w > best)
			best = w;
		lines++;
	}
	return (best);
}

/* Print the rounded top border with `title` set into it, spanning `cols`. The
   title splits on its first space: the name is salmon, the version grey. */
void	title_rule(const char *title, int cols)
{
	t_glyphs	g;
	char		buf[256];
	char		*sp;
	int			fill;
	int			w;

	g = header_glyphs();
	if (!title)
		title = "";
	header_clip(buf, sizeof(buf), title, cols - 8);
	w = header_width(buf);
	sp = ft_strchr(buf, ' ');
	if (sp)
		*sp = '\0';
	ft_eprintf(C_FRAME "%s%s%s%s ", g.tl, g.h, g.h, g.h);
	ft_eprintf(C_TITLE "%s" C_RST, buf);
	if (sp)
		ft_eprintf(" " C_VER "%s", sp + 1);
	ft_eprintf(" " C_FRAME);
	fill = cols - w - 7;
	while (fill-- > 0)
		ft_eprintf("%s", g.h);
	ft_eprintf("%s" C_RST "\n", g.tr);
}
