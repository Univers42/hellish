/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   rl_header.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/06 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/07/06 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "rl_private.h"
#include <sys/ioctl.h>

/* The current prompt's header row (everything above the arrow line), kept with
   its \x1e elastic-fill marker so it can be re-rendered at any width; and the
   width it was last rendered at (to know how many physical rows the stale copy
   occupies after the terminal reflows on a zoom). Reset per readline child by
   the fork, so continuation prompts (no header) never reflow a stale one. */
static char	g_hdr[1024];
static int	g_hdr_cols;

/* The ─ run width for `cols`: whatever the fixed parts leave over. The two
   halves are measured without the marker (visible width: ANSI and control
   bytes cost 0, multibyte glyphs count cells). May be <= 0 on a terminal too
   narrow for the full header -- the caller falls back to clipping. */
static int	hdr_fill(int cols)
{
	char	tmp[1024];
	char	*mk;

	ft_strlcpy(tmp, g_hdr, sizeof(tmp));
	mk = ft_strchr(tmp, '\x1e');
	if (!mk)
		return (0);
	*mk = '\0';
	return (cols - visible_width_cstr(tmp) - visible_width_cstr(mk + 1));
}

/* Too narrow for content + clock: emit the left content as PLAIN text (ANSI
   and markers dropped, UTF-8 lead bytes cost 1 cell, continuation bytes ride
   with their lead) until 2 columns remain, then close the box at the exact
   right edge. The clock and colors are sacrificed at this width. */
static void	hdr_clip(int cols)
{
	int	i;
	int	left;

	left = cols - 2;
	i = 0;
	while (g_hdr[i] && g_hdr[i] != '\x1e')
	{
		if (g_hdr[i] == '\001' || g_hdr[i] == '\002')
			i++;
		else if ((unsigned char)g_hdr[i] == 0x1b)
		{
			while (g_hdr[i] && !ft_isalpha(g_hdr[i]))
				i++;
			i += (g_hdr[i] != '\0');
		}
		else if (((unsigned char)g_hdr[i] & 0xc0) != 0x80 && left-- <= 0)
			break ;
		else
			fputc((unsigned char)g_hdr[i++], rl_outstream);
	}
	fprintf(rl_outstream, "\xe2\x94\x80\xe2\x95\xae\n");
}

/* Print the stored header at width `cols`: strip the \001/\002 width markers,
   substitute the elastic marker with the computed ─ run; clip when the fixed
   parts alone are wider than the terminal. */
static void	hdr_render(int cols)
{
	int	fill;
	int	i;

	fill = hdr_fill(cols);
	if (fill < 1)
		return (hdr_clip(cols));
	i = 0;
	while (g_hdr[i])
	{
		if (g_hdr[i] == '\x1e')
			while (fill-- > 0)
				fputs("\xe2\x94\x80", rl_outstream);
		else if (g_hdr[i] != '\001' && g_hdr[i] != '\002')
			fputc((unsigned char)g_hdr[i], rl_outstream);
		i++;
	}
	fflush(rl_outstream);
}

/* Store the header part of the prompt (up to and including its newline) and
   print it at the current width. Called once per prompt by split_prompt. */
void	rl_header_print(const char *prompt, size_t len)
{
	struct winsize	w;

	if (len >= sizeof(g_hdr))
		len = sizeof(g_hdr) - 1;
	ft_memcpy(g_hdr, prompt, len);
	g_hdr[len] = '\0';
	g_hdr_cols = 80;
	if (ioctl(1, TIOCGWINSZ, &w) == 0 && w.ws_col > 0)
		g_hdr_cols = w.ws_col;
	hdr_render(g_hdr_cols);
}

/* Re-render the header for the post-resize width: climb over the stale copy
   (which a reflowing terminal rewrapped to ceil(old/new) rows on narrowing),
   wipe from there down, and print the fresh row. Ends at column 0 of the
   arrow row so readline can repaint the input line below. 0 if there is no
   header to reflow (continuation prompts). ponytail: assumes the edit line
   was a single row pre-zoom; a wrapped edit line may leave a stray row. */
int	rl_header_reflow(void)
{
	struct winsize	w;
	int				cols;
	int				up;

	if (!g_hdr[0])
		return (0);
	cols = 80;
	if (ioctl(1, TIOCGWINSZ, &w) == 0 && w.ws_col > 0)
		cols = w.ws_col;
	up = (g_hdr_cols + cols - 1) / cols;
	fprintf(rl_outstream, "\r\033[%dA\033[J", up);
	hdr_render(cols);
	g_hdr_cols = cols;
	return (1);
}
