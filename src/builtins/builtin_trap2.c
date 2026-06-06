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

/* g_trap_pending holds the signal number of the most recently received
   trapped signal. It is declared volatile sig_atomic_t so the C standard
   permits the write inside the async signal handler — reading from it in
   run_pending_traps (called from the REPL) is safe. We only keep one pending
   slot, which is fine for the common "catch Ctrl-C, clean up" pattern. */
static volatile sig_atomic_t	g_trap_pending = 0;

/* The actual OS signal handler. Only does the minimum safe work: record the
   signal number and return. Executing shell code here would re-enter the
   malloc/printf machinery and cause undefined behaviour. */
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
	xfree(cmd);
}

/* trap -p [condition ...] : print the trap commands for the named conditions
   (or every set trap when none are named), in a re-readable form. */
int	print_traps_for(t_shell *state, t_vec argv)
{
	char	**av;
	size_t	i;
	int		num;

	av = (char **)argv.ctx;
	if (argv.len == 2)
		return (list_traps(state));
	i = 1;
	while (++i < argv.len)
	{
		num = trap_sig_from_name(av[i]);
		if (num >= 0 && state->traps[num])
			ft_printf("trap -- '%s' %s\n", state->traps[num],
				sig_to_name(num));
	}
	return (0);
}

/* Register or remove one trap. action "-" restores the default signal
   disposition; action "" ignores the signal (SIG_IGN); anything else stores
   the string and registers trap_sighandler. EXIT (num == 0) is not an OS
   signal, so we never call signal() for it — it is triggered manually by
   exit_clean(). Frees any previous action string first to avoid leaks. */
int	set_one_trap(t_shell *state, const char *action, int num)
{
	if (num < 0 || num >= 32)
		return (1);
	xfree(state->traps[num]);
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
