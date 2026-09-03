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
#include "env.h"
#include "expander.h"

/* Borrow this command's argv backing from the depth-indexed pool: reuse the
   slot's array (just reset its length) so a simple command does no per-command
   malloc. Past ARGV_POOL_DEPTH (deep recursion) fall back to a fresh vector.
     The slot is EMPTIED while the array is out.  It used to keep pointing at
   the buffer it lent, and the borrower may realloc that buffer -- a glob
   that expands to more words than the array held does -- after which the
   slot held a freed pointer.  Nothing read it until the shell tore down
   with the command still in flight: a pipeline child whose command was not
   found (`true | nosuch *.md`) freed the pool on its way out and died of a
   double free instead of exiting 127.  With the slot empty until release,
   the pool only ever frees what it actually holds. */
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
		slot->ctx = NULL;
		slot->cap = 0;
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

/* Free every backing array PARKED in the pool (once, at shutdown).  An
   array still out on loan belongs to its command and is not here. */
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

/* Release one expanded command. pre_assigns are KEY=value pairs pushed before
   the command (e.g. `FOO=bar cmd`); each needs both key and value freed. argv
   is a vec of word-slab pointers, so we go through word_free() rather than
   xfree() -- both paths (slab and heap) route correctly. The argv backing
   array returns to the pool via argv_pool_release. */
void	free_executable_cmd(t_shell *state, t_executable_cmd cmd)
{
	size_t	i;
	t_env	*e;

	i = -1;
	while (++i < cmd.pre_assigns.len)
	{
		e = &((t_env *)cmd.pre_assigns.ctx)[i];
		xfree(e->value);
		xfree(e->key);
	}
	i = -1;
	while (++i < cmd.argv.len)
		word_free(((char **)cmd.argv.ctx)[i]);
	xfree(cmd.pre_assigns.ctx);
	argv_pool_release(state, &cmd);
}

/* Release the redirect list embedded in an executable AST node. The node itself
   is stack-allocated by the executor, so only its heap-allocated children
   (redirs.ctx) need freeing. vec_init resets it to a safe empty state. */
void	free_executable_node(t_executable_node *node)
{
	xfree(node->redirs.ctx);
	vec_init(&node->redirs);
}
