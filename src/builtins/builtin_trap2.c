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

/* Run a queued signal trap (called from the REPL between commands).  $? is
   saved across the handler: POSIX says a trap action must not disturb the
   status the surrounding script observes (`( exit 42 )` inside a USR1 trap
   leaves the next `$?` untouched -- bash and dash both isolate it). */
void	run_pending_traps(t_shell *state)
{
	t_execution_state	saved;
	int					sig;

	sig = (int)g_trap_pending;
	g_trap_pending = 0;
	if (sig <= 0 || sig >= SH_NSIG || !state->traps[sig])
		return ;
	saved = state->last_cmd_st_exe;
	exec_string(state, state->traps[sig]);
	set_cmd_status(state, saved);
}

/* Run the EXIT trap (called once when the shell terminates).  Popping the
   action first is the recursion guard: an `exit` inside the trap body calls
   exit_clean -> run_exit_trap again and finds the slot empty.  should_exit
   is cleared for the duration because it is already true on every path that
   gets here, and the executor's list/loop walkers treat it as "stop after
   the current command" -- without the reset a multi-command trap body like
   'echo bye; exit 42' would silently drop everything after its first
   command.  A nested `exit` still terminates instantly via exit_clean's
   direct exit(), so clearing the flag cannot resurrect the session. */
void	run_exit_trap(t_shell *state)
{
	char	*cmd;

	cmd = state->traps[0];
	if (!cmd)
		return ;
	state->traps[0] = NULL;
	state->should_exit = false;
	exec_string(state, cmd);
	state->should_exit = true;
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
			print_one_trap(state, num);
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
	if (num < 0 || num >= SH_NTRAP)
		return (1);
	xfree(state->traps[num]);
	state->traps[num] = NULL;
	if (action[0] == '-')
	{
		if (num > 0 && num < SH_NSIG)
			pal_trap_dfl(state, num);
		return (0);
	}
	state->traps[num] = ft_strdup(action);
	if (num > 0 && num < SH_NSIG && action[0] == '\0')
		pal_trap_ign(state, num);
	else if (num > 0 && num < SH_NSIG)
		pal_trap_arm(state, num, trap_sighandler);
	return (0);
}
