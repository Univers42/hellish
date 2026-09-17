/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   rl_winch.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/17 03:30:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/09/17 04:10:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "rl_private.h"
#include <sys/ioctl.h>

/* A resize while a line is being edited -- readline's handling, replaced.
**
** Field report: clicking from terminal tab to terminal tab printed another
** prompt each time, and zooming stacked right-prompt clocks:
**
**     ╰─ ❯ ╰─ ❯ ╰─ ❯ ╰─ ❯ ╰─ ❯                        03:003:00
**
** Tab switches and zooms raise SIGWINCH, often with the size unchanged.
**
** readline's resize path ends in rl_resize_terminal(), which -- as soon as
** rl_redisplay_function is not rl_redisplay itself, and the RPROMPT painter
** makes it rl_rprompt's -- calls rl_forced_update_display() directly. That
** redraws the line from "column 0" without first carrying the cursor
** there, so every SIGWINCH appended one more copy of the prompt and line
** after the last. Installing a redisplay function is what arms it, so this
** belongs with the painter rather than with either reader.
**
** So readline catches no SIGWINCH (rl_catch_sigwinch = 0). The handler
** below only records one, and the redraw happens where readline waits for
** a key: rl_signal_event_hook, which rl_getc_hook calls after a signal
** interrupts its pselect, and the animation's idle tick, which is the
** other wait.
**
** Only a change of COLUMNS redraws. A tab switch or a change of LINES
** moves nothing on the line, and a redraw that is not needed is exactly
** where the flicker and the leftovers come from. */

/* The resize state for one read. A cell rather than t_shell, because the
   signal handler writes it. */
t_winch	*rl_winch_cell(void)
{
	static t_winch	st;

	return (&st);
}

/* Record the resize, and chain to what was installed (a `trap WINCH`). */
static void	winch_handler(int sig)
{
	t_winch	*st;

	st = rl_winch_cell();
	st->winch = 1;
	if (st->old.sa_handler != SIG_DFL && st->old.sa_handler != SIG_IGN)
		st->old.sa_handler(sig);
}

/* The terminal's current width, or -1. */
int	rl_winch_cols(void)
{
	struct winsize	ws;

	if (ioctl(fileno(rl_outstream), TIOCGWINSZ, &ws) != 0 || ws.ws_col == 0)
		return (-1);
	return (ws.ws_col);
}

/* Take SIGWINCH for one readline call. */
void	rl_winch_arm(void)
{
	struct sigaction	act;
	t_winch				*st;

	st = rl_winch_cell();
	st->winch = 0;
	st->cols = rl_winch_cols();
	ft_memset(&act, 0, sizeof(act));
	act.sa_handler = winch_handler;
	sigemptyset(&act.sa_mask);
	sigaction(SIGWINCH, &act, &st->old);
	rl_catch_sigwinch = 0;
	rl_signal_event_hook = rl_winch_poll;
}

/* Give SIGWINCH back, as readline's default expects to find it. */
void	rl_winch_disarm(void)
{
	sigaction(SIGWINCH, &rl_winch_cell()->old, NULL);
	rl_signal_event_hook = NULL;
	rl_catch_sigwinch = 1;
}
