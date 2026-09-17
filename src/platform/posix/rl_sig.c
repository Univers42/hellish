/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   rl_sig.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/17 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/09/17 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "rl_private.h"
#include "zle.h"

/* Why a read ends early. */

/* Set when readline handled a SIGINT during the current read. Written
   from the reader, never from a signal handler: readline defers the
   signal and our reader is where it is handled (rl_take_signals). */
int	*rl_intr_cell(void)
{
	static int	intr;

	return (&intr);
}

/* Set by shell code run from the editor that means the editor must stop:
   a widget that ran `exit` (rl_shell_exec). */
int	*rl_editor_abort_cell(void)
{
	static int	abort_read;

	return (&abort_read);
}

/* Stop reading: ^C was handled, the shell's own ^C handler asked to
   unwind, or shell code run from the editor asked to leave. */
bool	rl_should_abort(void)
{
	return (*rl_intr_cell() || *rl_editor_abort_cell()
		|| get_g_sig()->should_unwind);
}

/* READERR ends readline's main loop and makes readline() return NULL. A
   nested reader (incremental search, a quoted insert, vi's pending
   operator) is not the main loop and gets EOF, which each of them takes
   as "give up"; readline then comes back to the main loop and asks us
   again. */
int	rl_abort_value(void)
{
	if (RL_ISSTATE(RL_STATE_READCMD))
		return (READERR);
	return (EOF);
}

/* A fresh read starts with nothing to abort for. */
void	rl_abort_reset(void)
{
	*rl_intr_cell() = 0;
	*rl_editor_abort_cell() = 0;
}
