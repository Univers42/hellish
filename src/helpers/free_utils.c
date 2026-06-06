/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   free_utils.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 23:33:27 by marvin            #+#    #+#             */
/*   Updated: 2026/01/09 23:33:27 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "shell.h"
#include <unistd.h>
#include "env.h"
#include "expander.h"

void	pos_free(t_pos *pos);

/* Tear down one command's redirect list. Three cleanup duties in one pass:
   (1) unlink() any here-doc tmpfiles (should_delete marks them), (2) close
   any file descriptor we opened for a persistent fd redir (is_dup and
   close_fd are the two cases where we do NOT own the fd), (3) free the
   filename string. Finally reset the vec to a pristine zero-length state so
   the next command can vec_push into it without re-initialising. */
void	free_redirects(t_vec_redir *v)
{
	size_t	i;
	t_redir	c;

	i = 0;
	while (i < v->len)
	{
		c = ((t_redir *)v->ctx)[i];
		if (c.should_delete)
			unlink(c.fname);
		if (!c.is_dup && !c.close_fd && c.fd > STDERR_FILENO)
			close(c.fd);
		xfree(c.fname);
		i++;
	}
	xfree(v->ctx);
	v->ctx = NULL;
	v->len = 0;
	v->cap = 0;
	v->elem_size = sizeof(t_redir);
}

/* Walk the function table and release each entry: the name string and the
   whole body AST. The vec's backing array itself is freed last, same pattern
   as every other vec-of-structs we own. Called once at shutdown from
   free_all_state. */
static void	free_functions(t_vec *fns)
{
	size_t			i;
	t_shell_func	*fn;

	i = 0;
	while (i < fns->len)
	{
		fn = vec_idx(fns, i);
		xfree(fn->name);
		free_ast(&fn->body);
		i++;
	}
	xfree(fns->ctx);
}

/* trap handler strings (ft_strdup'd in set_one_trap) live for the session and
   were never released at exit — free the whole table so a script using trap is
   leak-clean like any other. */
static void	free_traps(t_shell *state)
{
	int	i;

	i = -1;
	while (++i < 32)
	{
		xfree(state->traps[i]);
		state->traps[i] = NULL;
	}
}

/* The single canonical shutdown path. Order is deliberate: functions first
   (they may reference variables), then per-command scratch (input, AST,
   redirects, proc subs), then session data (history, cwd, positional args,
   the argv pool, traps). env_index_free() must come after all env lookups;
   word_slab_teardown() is last so any stray word_free() calls during the
   earlier steps still work. alloc_live_report() is the ft_malloc leak oracle
   -- it's a no-op on libc builds and a canary for the slab backend. */
void	free_all_state(t_shell *state)
{
	free_functions(&state->functions);
	xfree(state->input.ctx);
	state->input = (t_string){};
	xfree(state->last_cmd_st);
	xfree(state->pid);
	xfree(state->last_bg_pid);
	xfree(state->ctx);
	xfree(state->dft_ctx);
	state->ctx = 0;
	state->dft_ctx = 0;
	xfree(state->rl.buff.ctx);
	free_redirects(&state->redirects);
	cleanup_proc_subs(state);
	free_ast(&state->tree);
	free_hist(state);
	xfree(state->cwd.ctx);
	pos_free(&state->pos);
	free_argv_pool(state);
	free_traps(state);
	free_dirstack(state);
	env_index_free();
	word_slab_teardown();
	alloc_live_report();
}

/* Release one expanded command. pre_assigns are KEY=value pairs pushed
   before the command (e.g. `FOO=bar cmd`); each needs both key and value
   freed. argv is a vec of word-slab pointers, so we go through word_free()
   rather than xfree() -- both paths (slab and heap) route correctly. The
   argv backing array returns to the pool via argv_pool_release. */
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

/* Release the redirect list embedded in an executable AST node. The node
   itself is stack-allocated by the executor, so only its heap-allocated
   children (redirs.ctx) need freeing. vec_init resets it to a safe empty
   state so the node can be reused or safely inspected after freeing. */
void	free_executable_node(t_executable_node *node)
{
	xfree(node->redirs.ctx);
	vec_init(&node->redirs);
}
