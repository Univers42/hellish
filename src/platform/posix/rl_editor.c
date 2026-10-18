/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   rl_editor.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/17 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/09/17 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "rl_private.h"
#include "zle.h"
#include "pal.h"
#include <termios.h>

int	exec_string(t_shell *state, char *content);

/* Shell code run from inside the editor -- a widget, a completion
** function -- in the shell process, while readline owns the terminal.
**
** readline's signal handlers are taken down for the call, as bash does
** around `bind -x`, so ^C reaches the shell's own handler and stops a
** loop in the widget instead of being queued for readline; rl_set_signals
** puts them back and records whatever handler is current then -- a `trap
** ... INT` the widget set is the one readline forwards to afterwards.
**
** The terminal is put back to readline's settings: a widget may run
** `stty`, or a command whose job control hands the terminal back in
** cooked mode. TCSANOW, never a deprep/prep pair, which would echo
** typeahead. $? is the user's again afterwards, and a widget that ran
** `exit` ends the read (rl_should_abort).
**
** readline's stream is buffered (rl_outstream.c): what it drew goes out
** first, so the widget's own output lands after it and a child the widget
** forks never inherits bytes still in that buffer. */
void	rl_shell_enter(t_shell *state, t_rl_bracket *b)
{
	fflush(rl_outstream);
	b->status = state->last_cmd_st_exe;
	b->have_tty = (tcgetattr(STDIN_FILENO, &b->tty) == 0);
	rl_clear_signals();
}

/* 0 when the shell code left the editor for good (`exit` from a widget
   that did not exit, an exec that failed after leaving). */
int	rl_shell_leave(t_shell *state, t_rl_bracket *b)
{
	if (!zle_active())
		return (0);
	rl_set_signals();
	if (b->have_tty)
		tcsetattr(STDIN_FILENO, TCSANOW, &b->tty);
	rl_reset_screen_size();
	set_cmd_status(state, b->status);
	if (state->should_exit)
		*rl_editor_abort_cell() = 1;
	return (1);
}

int	rl_shell_exec(t_shell *state, char *code)
{
	t_rl_bracket	b;
	int				rc;

	rl_shell_enter(state, &b);
	rc = exec_string(state, code);
	rl_shell_leave(state, &b);
	return (rc);
}

/* The process is about to leave the editor for good -- `exit`, or `exec`
   replacing it -- from inside a widget: give the terminal back as
   readline found it. Nothing to do outside the editor. */
void	pal_editor_leave(void)
{
	if (!zle_active())
		return ;
	rl_clear_signals();
	(*rl_deprep_term_function)();
	zle_enter(NULL);
}
