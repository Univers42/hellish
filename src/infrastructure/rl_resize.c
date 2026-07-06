/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   rl_resize.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/06 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/07/06 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "rl_private.h"
#include "rl_ghost_ai.h"
#include <signal.h>

/* Set from the SIGWINCH handler, consumed in ghost_redisplay. A resize must
   never be serviced from signal context. */
static volatile sig_atomic_t	g_winch;

static void	winch_handler(int sig)
{
	(void)sig;
	g_winch = 1;
}

/* Take SIGWINCH away from readline. On a resize readline repositions the cursor
   (\r + erase) before repainting ONLY for its default redisplay; with our ghost
   redisplay installed it skips that and repaints the arrow wherever the cursor
   happens to sit, stacking a fresh copy on every resize -- the "arrow
   duplicates infinitely on zoom" bug. ghost_redisplay does the clean repaint
   itself when rl_resize_take() reports a pending resize. */
void	rl_resize_setup(void)
{
	struct sigaction	sa;

	rl_catch_sigwinch = 0;
	ft_bzero(&sa, sizeof(sa));
	sa.sa_handler = winch_handler;
	sigaction(SIGWINCH, &sa, NULL);
}

/* 1 (once) if a resize is pending; clears the flag. */
static int	rl_resize_take(void)
{
	if (!g_winch)
		return (0);
	g_winch = 0;
	return (1);
}

/* Service a pending resize: reflow the header to the new width (or just
   reposition when this prompt has none), re-read the terminal size, and mark a
   fresh line so readline repaints the arrow row once, in place, instead of
   stacking a copy wherever the cursor sat. Runs from the signal event hook
   (live, while readline waits for input) and from ghost_redisplay (belt and
   suspenders on the next keystroke). At most once per resize; returns 1 when
   a resize was actually handled so the caller knows to force a repaint. */
int	rl_resize_fixup(void)
{
	if (!rl_resize_take())
		return (0);
	if (!rl_header_reflow())
		fputs("\r\033[K", rl_outstream);
	rl_reset_screen_size();
	rl_on_new_line();
	return (1);
}
