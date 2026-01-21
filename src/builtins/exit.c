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

#include "builtins_private.h"

void exit_clean(t_shell *state, int code)
{
	char *pid_s;

	pid_s = getpid_hack();
	if (pid_s && state->pid && ft_strcmp(state->pid, pid_s) == 0)
	{
		manage_history(state);
		free_all_state(state);
	}
	free(pid_s);
	exit(code);
}

int builtin_exit(t_shell *state, t_vec argv)
{
	int		ret;
	size_t	i;

	if (state->input_method == INP_READLINE)
		ft_eprintf("exit\n");
	/* no arguments => use last command status */
	if (argv.len == 1)
		exit_clean(state, state->last_cmd_status_res.status);
	i = 1;
	/* handle optional "--" */
	if (i < argv.len && ft_strcmp(((char **)argv.ctx)[i], "--") == 0)
	{
		if (i + 1 == argv.len)
			exit_clean(state, state->last_cmd_status_res.status);
		i++;
	}
	/* first argument must be numeric */
	if (ft_checked_atoi(((char **)argv.ctx)[i], &ret, 42))
	{
		ft_eprintf("%s: %s: %s: numeric argument required\n", state->context,
			((char **)argv.ctx)[0], ((char **)argv.ctx)[i]);
		exit_clean(state, 2);
	}
	/* too many arguments: match bash by exiting with status 1 */
	if (i + 1 < argv.len)
	{
		ft_eprintf("%s: %s: too many arguments\n", state->context,
			((char **)argv.ctx)[0]);
		exit_clean(state, 1);
	}
	exit_clean(state, ret);
	return (0);
}
