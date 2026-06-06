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

/* Too many arguments: bash exits with 1 (not 2, not the bad-arg status).
   The exit happens inside exit_clean, so the caller's return value is
   technically unreachable — but we return 1 anyway for readability. */
int	handle_too_many_args(t_shell *state, t_vec argv, size_t i)
{
	if (i + 1 < argv.len)
	{
		ft_eprintf("%s: %s: too many arguments\n", state->ctx,
			((char **)argv.ctx)[0]);
		return (exit_clean(state, 1), 1);
	}
	return (0);
}

/* Clean up and exit the process. Runs the EXIT trap first (only once: the
   trap sets traps[0]=NULL so a recursive exit from inside the trap body does
   not loop). History is persisted and state freed only in the top-level shell
   process — the pid comparison guards against subshell exits doing that work
   twice and corrupting the history file. */
void	exit_clean(t_shell *state, int code)
{
	char	*pid_s;

	run_exit_trap(state);
	pid_s = xgetpid();
	if (pid_s && state->pid && ft_strcmp(state->pid, pid_s) == 0)
		(manage_history(state), free_all_state(state));
	(xfree(pid_s), exit(code));
}
