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

static t_signame	*trap_sig_table(void)
{
	static t_signame	t[] = {{"EXIT", 0}, {"HUP", SIGHUP},
	{"INT", SIGINT}, {"QUIT", SIGQUIT}, {"TERM", SIGTERM}, {"USR1", SIGUSR1},
	{"USR2", SIGUSR2}, {"ALRM", SIGALRM}, {"PIPE", SIGPIPE},
	{"TSTP", SIGTSTP}, {NULL, -1}};

	return (t);
}

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

int	list_traps(t_shell *state)
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
