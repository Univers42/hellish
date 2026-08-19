/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   exit.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 23:29:54 by marvin            #+#    #+#             */
/*   Updated: 2026/01/09 23:29:54 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"
#include <unistd.h>

/* Clean up and exit the process. Runs the EXIT trap first (only once: the
   trap sets traps[0]=NULL so a recursive exit from inside the trap body does
   not loop). History is persisted and state freed only in the top-level shell
   process — the pid comparison guards against subshell exits doing that work
   twice and corrupting the history file. */
void	exit_clean(t_shell *state, int code)
{
	char	*pid_s;

	run_exit_trap(state);
	pid_s = ft_itoa((int)getpid());
	if (pid_s && state->pid && ft_strcmp(state->pid, pid_s) == 0)
		(manage_history(state), free_all_state(state));
	(xfree(pid_s), exit(code));
}

/* The status a FATAL shell error exits with.  bash uses 127 only when the
   top-level shell built from a -c string dies; a script, piped stdin, or
   any forked child (a subshell, a command substitution) uses 1.  Measured
   on bash 5.3.9 for all three fatal families that share this: ${p:?w},
   `set -u` on an unset name, and assignment to a read-only variable.
   The pid comparison is the same one exit_clean uses to tell the original
   process from a subshell copy of t_shell. */
int	shell_fatal_status(t_shell *state)
{
	char	*pid_s;
	bool	original;

	pid_s = ft_itoa((int)getpid());
	original = (pid_s && state->pid && ft_strcmp(state->pid, pid_s) == 0);
	xfree(pid_s);
	if (state->metinp == INP_ARG && original)
		return (127);
	return (1);
}
