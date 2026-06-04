/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   res_utils.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 23:34:40 by marvin            #+#    #+#             */
/*   Updated: 2026/01/09 23:34:40 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "execution_private.h"

t_execution_state	res_status(int status)
{
	return ((t_execution_state){.status = status, .pid = -1});
}

t_execution_state	res_pid(int pid)
{
	return ((t_execution_state){.status = -1, .pid = pid});
}

void	exe_res_set_status(t_execution_state *res)
{
	int	status;

	if (res->status != -1)
		return ;
	ft_assert(res->pid != -1);
	while (1)
		if (waitpid(res->pid, &status, 0) != -1)
			break ;
	if (WIFSIGNALED(status) && WTERMSIG(status) == SIGINT)
		res->ctrl_c = true;
	res->status = WEXITSTATUS(status)
		+ WIFSIGNALED(status) * 128 + WIFSIGNALED(status) * WTERMSIG(status);
}

/* `set -o pipefail`: wait every child (reaping them) and return the rightmost
   non-zero exit, or 0 if all succeeded. */
static t_execution_state	pipefail_scan(t_vec_exe_res *results)
{
	t_execution_state	*r;
	t_execution_state	res;
	size_t				i;

	res = res_status(0);
	i = 0;
	while (i < results->len)
	{
		r = (t_execution_state *)vec_idx(results, i);
		if (r->pid != -1)
			exe_res_set_status(r);
		if (r->status != 0)
			res = *r;
		i++;
	}
	return (res);
}

/* Exit status of a pipeline: pipefail (above), else the last command. */
t_execution_state	pipeline_status(t_shell *state, t_vec_exe_res *results)
{
	t_execution_state	res;

	if (state->opt_pipefail)
		return (pipefail_scan(results));
	res = *(t_execution_state *)vec_idx(results, results->len - 1);
	if (res.pid != -1)
		exe_res_set_status(&res);
	return (res);
}
