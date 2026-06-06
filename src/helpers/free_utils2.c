/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   free_utils2.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/05 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/05 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "shell.h"
#include "executor.h"

/* Borrow this command's argv backing from the depth-indexed pool: reuse the
   slot's array (just reset its length) so a simple command does no per-command
   malloc. Past ARGV_POOL_DEPTH (deep recursion) fall back to a fresh vector. */
void	argv_pool_acquire(t_shell *state, t_executable_cmd *cmd)
{
	t_vec	*slot;

	if (state->argv_pool_depth < ARGV_POOL_DEPTH)
	{
		slot = &state->argv_pool[state->argv_pool_depth];
		slot->len = 0;
		slot->elem_size = sizeof(char *);
		cmd->argv = *slot;
		cmd->pooled = true;
		state->argv_pool_depth++;
	}
	else
	{
		vec_init(&cmd->argv);
		cmd->argv.elem_size = sizeof(char *);
		cmd->pooled = false;
	}
}

/* Return the argv backing: park the (possibly grown) array back in the slot for
   the next command to reuse, or free it for the non-pooled fallback. Strictly
   LIFO with argv_pool_acquire, so the depth counter stays balanced. */
void	argv_pool_release(t_shell *state, t_executable_cmd *cmd)
{
	t_vec	*slot;

	if (!cmd->pooled)
	{
		xfree(cmd->argv.ctx);
		return ;
	}
	state->argv_pool_depth--;
	slot = &state->argv_pool[state->argv_pool_depth];
	slot->ctx = cmd->argv.ctx;
	slot->cap = cmd->argv.cap;
	slot->len = 0;
	slot->elem_size = sizeof(char *);
}

/* Free every backing array held by the pool (once, at shutdown). */
void	free_argv_pool(t_shell *state)
{
	int	i;

	i = 0;
	while (i < ARGV_POOL_DEPTH)
	{
		xfree(state->argv_pool[i].ctx);
		state->argv_pool[i] = (t_vec){0};
		i++;
	}
}
