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

#include "builtins.h"

void	exit_clean(t_state *state, int code)
{
	char	*pid_s;

	pid_s = getpid_hack();
	if (pid_s && state->pid && ft_strcmp(state->pid, pid_s) == 0)
	{
		manage_history(state);
		free_all_state(state);
	}
	free(pid_s);
	exit(code);
}

int	builtin_exit(t_state *state, t_vec argv)
{
	int	ret;

	if (state->input_method == INP_READLINE)
		ft_eprintf("exit\n");
	if (argv.len == 1)
		exit_clean(state, 0);
	if (ft_checked_atoi(((char **)argv.ctx)[1], &ret, 42))
	{
		ft_eprintf("%s: %s: %s: numeric argument required\n", state->context,
			((char **)argv.ctx)[0], ((char **)argv.ctx)[1]);
		exit_clean(state, 2);
	}
	if (argv.len >= 3)
	{
		ft_eprintf("%s: %s: too many arguments\n", state->context,
			((char **)argv.ctx)[0]);
		return (1);
	}
	exit_clean(state, ret);
	return (0);
}
