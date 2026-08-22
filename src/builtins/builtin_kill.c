/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtin_kill.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/02 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/02 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"
#include "job_control.h"
#include <signal.h>
#include <errno.h>
#include <string.h>

/* The kill builtin has its own signal table (separate from trap's) because
   it needs a much wider set of signal names — KILL, STOP, CHLD, CONT, etc.
   that are not meaningful as trap conditions. t_signame is declared in
   ft_builtins.h and shared with builtin_trap.c. */
static const t_signame	*sig_table(void)
{
	static const t_signame	t[] = {
	{"HUP", SIGHUP}, {"INT", SIGINT}, {"QUIT", SIGQUIT}, {"ILL", SIGILL},
	{"TRAP", SIGTRAP}, {"ABRT", SIGABRT}, {"BUS", SIGBUS}, {"FPE", SIGFPE},
	{"KILL", SIGKILL}, {"USR1", SIGUSR1}, {"SEGV", SIGSEGV}, {"USR2", SIGUSR2},
	{"PIPE", SIGPIPE}, {"ALRM", SIGALRM}, {"TERM", SIGTERM}, {"CHLD", SIGCHLD},
	{"CONT", SIGCONT}, {"STOP", SIGSTOP}, {"TSTP", SIGTSTP}, {"TTIN", SIGTTIN},
	{"TTOU", SIGTTOU}, {"URG", SIGURG}, {"WINCH", SIGWINCH}, {NULL, 0}
	};

	return (t);
}

/* "TERM", "SIGTERM" or "15" -> signal number, or -1 when unknown. */
int	kill_sig_from_name(const char *name)
{
	const t_signame	*t;

	if (name[0] >= '0' && name[0] <= '9')
		return (ft_atoi(name));
	if (!ft_strncmp(name, "SIG", 3))
		name += 3;
	t = sig_table();
	while (t->name)
	{
		if (!ft_strcmp(t->name, name))
			return (t->num);
		t++;
	}
	return (-1);
}

/* kill -l: list every known signal in `N) SIGNAME` format, matching bash. */
int	kill_list_sigs(void)
{
	const t_signame	*t;

	t = sig_table();
	while (t->name)
	{
		ft_printf("%2d) SIG%s\n", t->num, t->name);
		t++;
	}
	return (0);
}

/* A literal pid: optional leading '-' then digits only. */
static bool	kill_is_pid(const char *s)
{
	if (*s == '-')
		s++;
	if (!*s)
		return (false);
	while (*s)
		if (!ft_isdigit(*s++))
			return (false);
	return (true);
}

/* %job -> whole process group (negative pgid); otherwise a literal pid. */
int	kill_one_target(t_shell *state, const char *target, int sig)
{
	t_job	*job;

	if (target[0] == '%')
	{
		job = job_by_spec(&state->job_table, target);
		if (!job)
			return (ft_eprintf("%s: kill: %s: no such job\n",
					state->ctx, target), 1);
		if (job_kill_group(state, job, sig) == -1)
			return (ft_eprintf("%s: kill: (%d): %s\n", state->ctx,
					job->pgid, strerror(errno)), 1);
		return (0);
	}
	if (!kill_is_pid(target))
		return (ft_eprintf("%s: kill: %s: arguments must be process or"
				" job IDs\n", state->ctx, target), 1);
	if (pal_kill(state, (pid_t)ft_atoi(target), sig) == -1)
		return (ft_eprintf("%s: kill: (%s): %s\n", state->ctx,
				target, strerror(errno)), 1);
	return (0);
}
