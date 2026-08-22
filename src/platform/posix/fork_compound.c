/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   fork_compound.c                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/28 15:20:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/07/28 15:20:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "execution_private.h"

/* POSIX process-creation leaf for a compound command that must not touch
   the parent.  The child resets signal handlers to defaults (they were
   SIG_IGN for job control), applies the node's redirects, and runs the
   compound body in-process; the parent only records the pid.  The child
   flips modify_parent_ctx to true because from its own point of view it
   now owns its context -- nested compounds run in place there. */
t_execution_state	fork_compound(t_shell *state, t_executable_node *exe)
{
	int	pid;

	pid = fork();
	if (pid == 0)
	{
		jc_child(state);
		default_signal_handlers();
		set_up_redirection(state, exe);
		exe->modify_parent_ctx = true;
		exit(run_compound(state, exe).status);
	}
	jc_parent(state, pid);
	return (res_pid(pid));
}
