/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   intro.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/03 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/03 13:00:21 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "header.h"
#include "update.h"
#include <unistd.h>
#include <stdlib.h>

/* Map one UTF-8 rune to its semantic colour (the blog's "colour roles"):
   body blocks tint green, the eye/blink white, the spark yellow, spaces
   reset. `*len` receives the rune's byte length. */
static const char	*rune_color(const char *s, int *len)
{
	unsigned char	c;

	c = (unsigned char)*s;
	if (c < 0x80)
		*len = 1;
	else if (c < 0xe0)
		*len = 2;
	else if (c < 0xf0)
		*len = 3;
	else
		*len = 4;
	if (*s == ' ')
		return ("\033[0m");
	if (*s == 'o' || *s == 'O')
		return ("\033[38;5;231m");
	if (*s == '*')
		return ("\033[1;38;5;226m");
	if (*s == '~')
		return ("\033[38;5;208m");
	return ("\033[38;5;77m");
}

/* Emit one frame line: pad to centre, then walk runes grouping consecutive
   same-colour glyphs into a single coloured segment (runtime colorisation). */
static void	emit_line(const char *s, int pad)
{
	const char	*cur;
	const char	*col;
	char		g[5];
	int			len;

	len = pad;
	while (len-- > 0)
		ft_eprintf(" ");
	cur = (void *)0;
	while (*s)
	{
		col = rune_color(s, &len);
		if (col != cur)
		{
			ft_eprintf("%s", col);
			cur = col;
		}
		ft_memcpy(g, s, len);
		g[len] = '\0';
		ft_eprintf("%s", g);
		s += len;
	}
	ft_eprintf("\033[0m\n");
}

/* Repaint one frame: home the cursor, clear, drop two lines, then the art. */
static void	play_frame(const char **frame, int pad)
{
	int	i;

	ft_eprintf("\033[H\033[2J\n\n");
	i = -1;
	while (frame[++i])
		emit_line(frame[i], pad);
}

/* Skip the animation when it would be unsafe or unwanted: no tty, colours
   disabled, or explicitly opted out. Accessibility-first, like the Copilot CLI
   banner. After the first run we stay silent for a fast startup unless
   HELLISH_ANIM forces it -- "avoid animations after first use" per the blog. */
static int	intro_enabled(void)
{
	if (!isatty(STDERR_FILENO))
		return (0);
	if (getenv("HELLISH_NO_BANNER") || getenv("HELLISH_NO_ANIM"))
		return (0);
	if (getenv("NO_COLOR"))
		return (0);
	if (getenv("HELLISH_ANIM"))
		return (1);
	return (!intro_seen());
}

/* Play the short entrance loop (~1.1s) before the static welcome panel. */
void	play_intro(void)
{
	const char	***frames;
	size_t		n;
	size_t		i;
	int			pad;
	int			reps;

	if (!intro_enabled())
		return ;
	frames = intro_frames(&n);
	pad = (header_cols() - 18) / 2;
	if (pad < 0)
		pad = 0;
	i = 0;
	reps = 0;
	while (reps < 2)
	{
		play_frame(frames[i % n], pad);
		usleep(95000);
		if (++i % n == 0)
			reps++;
	}
	intro_mark_seen();
}
