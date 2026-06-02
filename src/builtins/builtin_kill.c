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

/* t_signame is declared in ft_builtins.h (shared with builtin_trap.c). */
static const t_signame	*sig_table(void)
{
	static const t_signame	t[] = {
	{"HUP", SIGHUP}, {"INT", SIGINT}, {"QUIT", SIGQUIT}, {"ILL", SIGILL},
	{"TRAP", SIGTRAP}, {"ABRT", SIGABRT}, {"BUS", SIGBUS}, {"FPE", SIGFPE},
	{"KILL", SIGKILL}, {"USR1", SIGUSR1}, {"SEGV", SIGSEGV}, {"USR2", SIGUSR2},
	{"PIPE", SIGPIPE}, {"ALRM", SIGALRM}, {"TERM", SIGTERM}, {"CHLD", SIGCHLD},
	{"CONT", SIGCONT}, {"STOP", SIGSTOP}, {"TSTP", SIGTSTP}, {"TTIN", SIGTTIN},
	{"TTOU", SIGTTOU}, {"WINCH", SIGWINCH}, {NULL, 0}
	};

	return (t);
}

/* "TERM", "SIGTERM" or "15" -> signal number, or -1 when unknown. */
static int	sig_from_name(const char *name)
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

static int	kill_list(void)
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

/* A literal pid: optional leading '-' then digits only (never empty/garbage,
   so an unset $! can't collapse to "kill 0" and signal our whole group). */
static bool	is_pid(const char *s)
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
static int	kill_one(t_shell *state, const char *target, int sig)
{
	t_job	*job;

	if (target[0] == '%')
	{
		job = job_by_spec(&state->job_table, target);
		if (!job)
			return (ft_eprintf("%s: kill: %s: no such job\n",
					state->ctx, target), 1);
		if (kill(-job->pgid, sig) == -1)
			return (ft_eprintf("%s: kill: (%d): %s\n", state->ctx,
					job->pgid, strerror(errno)), 1);
		return (0);
	}
	if (!is_pid(target))
		return (ft_eprintf("%s: kill: %s: arguments must be process or"
				" job IDs\n", state->ctx, target), 1);
	if (kill((pid_t)ft_atoi(target), sig) == -1)
		return (ft_eprintf("%s: kill: (%s): %s\n", state->ctx,
				target, strerror(errno)), 1);
	return (0);
}

/* kill -l | kill [-s SIG | -SIG | -n NUM] target ...  (target = pid or %job) */
int	builtin_kill(t_shell *state, t_vec argv)
{
	char	**av;
	int		sig;
	int		i;
	int		ret;

	av = (char **)argv.ctx;
	if (argv.len >= 2 && !ft_strcmp(av[1], "-l"))
		return (kill_list());
	sig = SIGTERM;
	i = 1;
	if (argv.len > 1 && av[1][0] == '-' && av[1][1])
	{
		if ((!ft_strcmp(av[1], "-s") || !ft_strcmp(av[1], "-n")) && av[2])
			sig = sig_from_name(av[++i]);
		else
			sig = sig_from_name(av[1] + 1);
		i++;
	}
	if (sig < 0)
		return (ft_eprintf("%s: kill: %s: invalid signal\n",
				state->ctx, av[1]), 1);
	if (i >= (int)argv.len)
		return (ft_eprintf("%s: kill: usage: kill [-s sig|-n num|-sig]"
				" pid|%%job ...\n", state->ctx), 1);
	ret = 0;
	while (i < (int)argv.len)
		ret |= kill_one(state, av[i++], sig);
	return (ret);
}
