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

static volatile sig_atomic_t	g_trap_pending = 0;

static const t_signame	*sig_table(void)
{
	static const t_signame	t[] = {{"EXIT", 0}, {"HUP", SIGHUP},
	{"INT", SIGINT}, {"QUIT", SIGQUIT}, {"TERM", SIGTERM}, {"USR1", SIGUSR1},
	{"USR2", SIGUSR2}, {"ALRM", SIGALRM}, {"PIPE", SIGPIPE},
	{"TSTP", SIGTSTP}, {NULL, -1}};

	return (t);
}

static int	sig_from_name(const char *s)
{
	const t_signame	*t;
	int				i;

	if (!ft_strncmp(s, "SIG", 3))
		s += 3;
	t = sig_table();
	i = -1;
	while (t[++i].name)
		if (!ft_strcmp(s, t[i].name))
			return (t[i].num);
	if (s[0] >= '0' && s[0] <= '9')
		return (ft_atoi(s));
	return (-1);
}

static const char	*sig_to_name(int num)
{
	const t_signame	*t;
	int				i;

	t = sig_table();
	i = -1;
	while (t[++i].name)
		if (t[i].num == num)
			return (t[i].name);
	return ("?");
}

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

static int	set_one_trap(t_shell *state, const char *action, int num)
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

static int	list_traps(t_shell *state)
{
	int	i;

	i = -1;
	while (++i < 32)
		if (state->traps[i])
			ft_printf("trap -- '%s' %s\n", state->traps[i], sig_to_name(i));
	return (0);
}

int	builtin_trap(t_shell *state, t_vec argv)
{
	char	**av;
	size_t	i;
	int		num;

	av = (char **)argv.ctx;
	if (argv.len == 1 || (argv.len == 2 && !ft_strcmp(av[1], "-p")))
		return (list_traps(state));
	if (argv.len < 3)
		return (1);
	i = 1;
	while (++i < argv.len)
	{
		num = sig_from_name(av[i]);
		if (num < 0)
			return (ft_eprintf("%s: trap: %s: invalid signal\n",
					state->ctx, av[i]), 1);
		set_one_trap(state, av[1], num);
	}
	return (0);
}
