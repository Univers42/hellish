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

// Too many arguments: match bash by exiting with status 1
int	handle_too_many_args(t_shell *state, t_vec argv, size_t i)
{
	if (i + 1 < argv.len)
	{
		ft_eprintf("%s: %s: too many arguments\n", state->ctx,
			((char **)argv.ctx)[0]);
		return (exit_clean(state, 1), 1);
	}
	return (0);
}

void	exit_clean(t_shell *state, int code)
{
	char	*pid_s;

	pid_s = xgetpid();
	if (pid_s && state->pid && ft_strcmp(state->pid, pid_s) == 0)
		(manage_history(state), free_all_state(state));
	(free(pid_s), exit(code));
}
