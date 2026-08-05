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

/* Close all open pipe fds on the parent side of registered process
   substitutions.  Called after the outer command has been forked so the
   parent's copy of the write-end (for >(cmd)) does not keep the child's
   pipe alive indefinitely when the command closes its copy. */
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

/* `exec` with a redirection to a process substitution keeps that substitution
   alive for the rest of the session: a persistent fd (1/2) holds it open,
   so the child only finishes once the shell exits. Detach such procsubs
   (pid = -1) so cleanup still closes our fd but never blocks in waitpid
   for a child the shell is feeding; the kernel reaps them at exit. */
void	procsub_detach_all(t_shell *state)
{
	size_t	i;

	if (!state->proc_subs.ctx)
		return ;
	i = 0;
	while (i < state->proc_subs.len)
	{
		((t_procsub_entry *)state->proc_subs.ctx)[i].pid = -1;
		i++;
	}
}

/* Reap all process substitution children and release their resources.
   Closes parent-side fds first (stops feeding >(cmd) children), then
   waitpid's for each child with pid > 0 (detached ones have pid == -1 and
   are skipped — the kernel will reap them at shell exit).  The proc_subs
   vec is reset so state is clean for the next command. */
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
			pal_waitpid(state, entry->pid, &status, 0);
			entry->pid = -1;
		}
		xfree(entry->path);
		i++;
	}
	xfree(state->proc_subs.ctx);
	vec_init(&state->proc_subs);
	state->proc_subs.elem_size = sizeof(t_procsub_entry);
}
