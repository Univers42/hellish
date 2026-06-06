/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtin_trap.c                                     :+:      :+:    :+:   */
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

/* The signal name table is stored in a static local so it is initialised
   exactly once and shared between every call (no heap allocation needed).
   The sentinel `{NULL, -1}` lets callers iterate without knowing the count.
   Numeric signal names (e.g. "15") fall through to ft_atoi — that way
   `trap '' 15` and `trap '' TERM` both work. */
static t_signame	*trap_sig_table(void)
{
	static t_signame	t[] = {{"EXIT", 0}, {"HUP", SIGHUP},
	{"INT", SIGINT}, {"QUIT", SIGQUIT}, {"TERM", SIGTERM}, {"USR1", SIGUSR1},
	{"USR2", SIGUSR2}, {"ALRM", SIGALRM}, {"PIPE", SIGPIPE},
	{"TSTP", SIGTSTP}, {NULL, -1}};

	return (t);
}

/* Translate "HUP", "SIGHUP", or "1" into the signal number. The "SIG"
   prefix is silently stripped so both forms are accepted. Returns -1 when
   the name is not recognised — callers treat that as a usage error. */
int	trap_sig_from_name(const char *s)
{
	t_signame	*t;
	int			i;

	if (!ft_strncmp(s, "SIG", 3))
		s += 3;
	t = trap_sig_table();
	i = -1;
	while (t[++i].name)
		if (!ft_strcmp(s, t[i].name))
			return (t[i].num);
	if (s[0] >= '0' && s[0] <= '9')
		return (ft_atoi(s));
	return (-1);
}

/* Signal number to short name (e.g. 2 -> "INT") for `trap -p` output and
   `trap --` re-readable format. Returns "?" for anything not in the table
   so the caller can still print something meaningful. */
char	*sig_to_name(int num)
{
	t_signame	*t;
	int			i;

	t = trap_sig_table();
	i = -1;
	while (t[++i].name)
		if (t[i].num == num)
			return ((char *)t[i].name);
	return ("?");
}

/* Print all active traps in `trap -- 'cmd' SIG` re-readable form, which
   lets a caller save and restore them: `saved=$(trap -p); …; eval "$saved"`.
   Signals with no handler (NULL) are silently skipped. */
int	list_traps(t_shell *state)
{
	int	i;

	i = -1;
	while (++i < 32)
		if (state->traps[i])
			ft_printf("trap -- '%s' %s\n", state->traps[i], sig_to_name(i));
	return (0);
}

/* trap [action condition ...]: set signal handlers or print them.
   `trap` alone lists all set traps. `trap -p [sig ...]` prints named ones.
   `trap action sig ...` registers `action` for each signal: an empty string
   ignores the signal (SIG_IGN), a literal "-" restores the default, and any
   other string registers trap_sighandler to queue it for async execution
   between commands (checked by run_pending_traps in the REPL). */
int	builtin_trap(t_shell *state, t_vec argv)
{
	char	**av;
	size_t	i;
	int		num;

	av = (char **)argv.ctx;
	if (argv.len == 1)
		return (list_traps(state));
	if (!ft_strcmp(av[1], "-p"))
		return (print_traps_for(state, argv));
	if (argv.len < 3)
		return (1);
	i = 1;
	while (++i < argv.len)
	{
		num = trap_sig_from_name(av[i]);
		if (num < 0)
			return (ft_eprintf("%s: trap: %s: invalid signal\n",
					state->ctx, av[i]), 1);
		set_one_trap(state, av[1], num);
	}
	return (0);
}
