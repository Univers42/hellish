/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   header_private.h                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/03 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/03 13:00:19 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef HEADER_PRIVATE_H
# define HEADER_PRIVATE_H

# include "header.h"
# include "shell.h"
# include <wchar.h>

/* The shared palette: a muted-salmon frame, salmon name, grey version, cool
   headings, soft text, a vivid salmon "warn" and a soft green "ok". */
# define C_FRAME "\033[38;5;173m"
# define C_TITLE "\033[1;38;5;209m"
# define C_VER   "\033[38;5;245m"
# define C_HEAD  "\033[1;38;5;81m"
# define C_TAG   "\033[38;5;250m"
# define C_WARN  "\033[1;38;5;203m"
# define C_OK    "\033[38;5;78m"
# define C_RST   "\033[0m"

/* Running state for header_clip: the destination, how far it is filled, its
   capacity, the columns used so far and the column budget, plus the multibyte
   decode state. */
typedef struct s_clip
{
	char		*dst;
	size_t		o;
	size_t		size;
	int			used;
	int			max;
	mbstate_t	st;
}	t_clip;

/* The two content-column widths of a rendered panel. */
typedef struct s_cols
{
	int	lw;
	int	rw;
}	t_cols;

/* Print `n` blank spaces. */
void		emit_spaces(int n);

/* Print exactly `width` columns of horizontal rule, in the frame colour. */
void		emit_rule_cell(int width);

/* Print `text` in `color`, padded or truncated to exactly `width` columns.
   align 0 = left, 1 = centred. */
void		emit_cell(const char *text, int width, int align,
				const char *color);

/* The widest line (display columns) in a NULL-terminated, marked array. */
int			panel_max_width(const char **lines);

/* Print the rounded top border with `title` set into it, spanning `cols`. */
void		title_rule(const char *title, int cols);

/* The line at index `i`, or NULL once the array runs out. */
const char	*panel_nth(const char **a, int i);

/* Print one body row: left cell, divider, right cell, with side borders. */
void		body_row(const t_panel *p, const char *l, const char *r, t_cols c);

/* Print the closing rounded border, spanning `cols` columns. */
void		bottom_rule(int cols);

/* Length in bytes of an ANSI CSI escape at `s` (0 if none) — so width and
   centring can step over embedded colours. */
int			skip_ansi(const char *s);

#endif
