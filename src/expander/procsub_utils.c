/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   procsub_utils.c                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 19:15:57 by marvin            #+#    #+#             */
/*   Updated: 2026/01/22 19:15:57 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"

void	procsub_close_fds_parent(t_shell *state)
{
	size_t			i;
	t_procsub_entry	*entry;

	if (!state->proc_subs.ctx || state->proc_subs.len == 0)
		return ;
	i = 0;
	while (i < state->proc_subs.len)
	{
		entry = &((t_procsub_entry *)state->proc_subs.ctx)[i];
		if (entry->fd >= 0)
		{
			close(entry->fd);
			entry->fd = -1;
		}
		i++;
	}
}

void	cleanup_proc_subs(t_shell *state)
{
	size_t			i;
	t_procsub_entry	*entry;
	int				status;

	if (!state->proc_subs.ctx)
		return ;
	procsub_close_fds_parent(state);
	i = 0;
	while (i < state->proc_subs.len)
	{
		entry = &((t_procsub_entry *)state->proc_subs.ctx)[i];
		if (entry->pid > 0)
		{
			waitpid(entry->pid, &status, 0);
			entry->pid = -1;
		}
		free(entry->path);
		i++;
	}
	free(state->proc_subs.ctx);
	vec_init(&state->proc_subs);
	state->proc_subs.elem_size = sizeof(t_procsub_entry);
}
