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
