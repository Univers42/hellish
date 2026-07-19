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
   Every nameable Linux signal 1..31 is present: bash/dash accept a trap on
   any of them -- even KILL and STOP, where the trap is stored but the kernel
   simply never delivers (the smoosh-suite regression).  Short names, because
   that is what `bash --posix` prints in trap listings. */
static t_signame	*trap_sig_table(void)
{
	static t_signame	t[] = {{"EXIT", 0}, {"HUP", SIGHUP},
	{"INT", SIGINT}, {"QUIT", SIGQUIT}, {"ILL", SIGILL}, {"TRAP", SIGTRAP},
	{"ABRT", SIGABRT}, {"BUS", SIGBUS}, {"FPE", SIGFPE}, {"KILL", SIGKILL},
	{"USR1", SIGUSR1}, {"SEGV", SIGSEGV}, {"USR2", SIGUSR2},
	{"PIPE", SIGPIPE}, {"ALRM", SIGALRM}, {"TERM", SIGTERM},
	{"STKFLT", SIGSTKFLT}, {"CHLD", SIGCHLD}, {"CONT", SIGCONT},
	{"STOP", SIGSTOP}, {"TSTP", SIGTSTP}, {"TTIN", SIGTTIN},
	{"TTOU", SIGTTOU}, {"URG", SIGURG}, {"XCPU", SIGXCPU},
	{"XFSZ", SIGXFSZ}, {"VTALRM", SIGVTALRM}, {"PROF", SIGPROF},
	{"WINCH", SIGWINCH}, {"IO", SIGIO}, {"PWR", SIGPWR},
	{"SYS", SIGSYS}, {NULL, -1}};

	return (t);
}

/* Translate "HUP", "SIGHUP", "hup" or "1" into the signal number.  Both
   bash and dash match signal names case-insensitively (`trap - int` works),
   so we do too, and the "SIG" prefix is stripped the same way.  A numeric
   spec must be all digits and in range 0..64 -- bash rejects "100" and
   "9x" as conditions, so no sloppy atoi here.  Returns -1 when the spec is
   not recognised; callers report it and keep processing (bash resets the
   valid conditions in the list even when one is bad). */
int	trap_sig_from_name(const char *s)
{
	t_signame	*t;
	int			i;

	if (ft_isdigit((unsigned char)s[0]))
	{
		if (trap_arg_is_reset(s))
			return (ft_atoi(s));
		return (-1);
	}
	if (ft_tolower((unsigned char)s[0]) == 's'
		&& ft_tolower((unsigned char)s[1]) == 'i'
		&& ft_tolower((unsigned char)s[2]) == 'g' && s[3])
		s += 3;
	t = trap_sig_table();
	i = -1;
	while (t[++i].name)
		if (!ft_strcasecmp(s, t[i].name))
			return (t[i].num);
	return (-1);
}

/* Signal number to short name (e.g. 2 -> "INT") for trap listings, which is
   the form `bash --posix` prints.  NULL for a signal with no name (the
   realtime range 32..64); print_one_trap falls back to the raw number. */
char	*sig_to_name(int num)
{
	t_signame	*t;
	int			i;

	t = trap_sig_table();
	i = -1;
	while (t[++i].name)
		if (t[i].num == num)
			return ((char *)t[i].name);
	return (NULL);
}

/* Print all active traps in `trap -- 'cmd' SIG` re-readable form, which
   lets a caller save and restore them: `saved=$(trap -p); …; eval "$saved"`.
   Signals with no handler (NULL) are silently skipped. */
int	list_traps(t_shell *state)
{
	int	i;

	i = -1;
	while (++i < SH_NSIG)
		if (state->traps[i])
			print_one_trap(state, i);
	return (0);
}

/* trap [--] [action condition ...]: set signal handlers or print them.
   `trap` (or `trap --`) alone lists all set traps; `trap -p [sig ...]`
   prints named ones.  A first operand that is an unsigned integer naming a
   valid signal switches to the POSIX obsolescent reset form: every operand
   is a condition to restore to its default (`trap 0 2` resets EXIT and
   SIGINT).  Otherwise the first operand is the action -- "-" resets, ""
   ignores, anything else registers the string -- applied to each following
   condition.  A leading "-word" (bad option, e.g. `trap -1`) or an action
   with no conditions is a usage error: status 2, and because trap is a
   special builtin, POSIX makes that fatal to a non-interactive shell --
   both bash --posix and dash stop the script there, with the EXIT trap
   still firing.  should_exit reproduces that abort. */
int	builtin_trap(t_shell *state, t_vec argv)
{
	char	**av;
	size_t	i;

	av = (char **)argv.ctx;
	if (argv.len > 1 && !ft_strcmp(av[1], "-p"))
		return (print_traps_for(state, argv));
	i = 1;
	if (i < argv.len && !ft_strcmp(av[i], "--"))
		i++;
	if (i >= argv.len)
		return (list_traps(state));
	if (trap_arg_is_reset(av[i]))
		return (trap_reset_all(state, argv, i));
	if ((av[i][0] == '-' && av[i][1]) || i + 1 >= argv.len)
	{
		ft_eprintf("%s: trap: usage: trap [-p] [[action]"
			" condition ...]\n", state->ctx);
		if (state->metinp != INP_RL)
			state->should_exit = true;
		return (2);
	}
	return (trap_set_all(state, argv, i));
}
