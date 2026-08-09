/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   header.h                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/03 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/03 13:00:22 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef HEADER_H
# define HEADER_H

# include <stddef.h>

/* Line markers, used as a one-byte prefix on a panel line to pick its style:
   a horizontal rule, a coloured logo line, an accented heading, a strong
   "needs attention" line (warn) or a calm "all good" line (ok). */
# define HP_RULE "\001"
# define HP_LOGO "\003"
# define HP_HEAD "\002"
# define HP_WARN "\004"
# define HP_OK   "\005"

/* The one glyph set every header shares. Picked at runtime: real Unicode box
   art when the locale is UTF-8, plain ASCII otherwise — so the same code looks
   right on any terminal. */
typedef struct s_glyphs
{
	const char	*tl;
	const char	*tr;
	const char	*bl;
	const char	*br;
	const char	*h;
	const char	*v;
	const char	*arrow;
	const char	*dot;
	const char	*ell;
	const char	*up;
}	t_glyphs;

/* A panel is a full-width rounded box with its title set into the top border,
   a left identity/logo column and a right tips/news column split by a vertical
   divider. The same box is reused everywhere; only the content changes. Each
   column is a NULL-terminated array of lines; a line may start with HP_RULE,
   HP_LOGO or HP_HEAD to change its style. */
typedef struct s_panel
{
	const char	*title;
	const char	**left;
	const char	**right;
	const char	*logo_color;
}	t_panel;

/* 1 if the active locale can render Unicode glyphs, 0 to fall back to ASCII. */
int			header_use_unicode(void);

/* The glyph set for the current terminal (Unicode or ASCII). */
t_glyphs	header_glyphs(void);

/* The terminal width in columns (defaults to 80 when unknown). */
int			header_cols(void);

/* Display width of a UTF-8 string, in terminal columns. */
int			header_width(const char *s);

/* Copy at most `max` columns of `src` into `dst`; returns the width used. */
int			header_clip(char *dst, size_t size, const char *src, int max);

/* Render the whole panel: a title-bearing top border, the two columns, and the
   closing border — spanning the full width and truncating to fit. */
void		render_panel(const t_panel *p);

#endif
