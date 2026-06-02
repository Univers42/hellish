/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtin_trap2.c                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/02 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/02 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"
#include "executor.h"
#include <signal.h>

static volatile sig_atomic_t	g_trap_pending = 0;

static void	trap_sighandler(int sig)
{
	g_trap_pending = sig;
}

/* Run a queued signal trap (called from the REPL between commands). */
void	run_pending_traps(t_shell *state)
{
	int	sig;

	sig = (int)g_trap_pending;
	g_trap_pending = 0;
	if (sig > 0 && sig < 32 && state->traps[sig])
		exec_string(state, state->traps[sig]);
}

/* Run the EXIT trap (called once when the shell terminates). */
void	run_exit_trap(t_shell *state)
{
	char	*cmd;

	cmd = state->traps[0];
	if (!cmd)
		return ;
	state->traps[0] = NULL;
	exec_string(state, cmd);
	free(cmd);
}

int	set_one_trap(t_shell *state, const char *action, int num)
{
	if (num < 0 || num >= 32)
		return (1);
	free(state->traps[num]);
	state->traps[num] = NULL;
	if (action[0] == '-')
	{
		if (num > 0)
			signal(num, SIG_DFL);
		return (0);
	}
	state->traps[num] = ft_strdup(action);
	if (num > 0 && action[0] == '\0')
		signal(num, SIG_IGN);
	else if (num > 0)
		signal(num, trap_sighandler);
	return (0);
}
