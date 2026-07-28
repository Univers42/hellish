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
#include "parena.h"

void	arr_marks_clear(t_shell *state);
void	attr_clear(t_shell *state);

void	pos_free(t_pos *pos);
void	free_positional_snapshot(t_vec *w);

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

/* Session-lifetime strings and caches, in the exact order free_all_state
   always released them: input buffer, alias-expansion scratch, the pid /
   $! / ctx strings, the readline buffer, then the split-$PATH cache (see
   path_cache_sync — free_tab is not NULL-safe, hence the guard, and the
   pointers are NULLed so a stray re-entry cannot double-free). */
static void	free_session_strings(t_shell *state)
{
	xfree(state->input.ctx);
	state->input = (t_string){};
	if (state->alias_exp_owned)
		xfree(state->alias_exp.ctx);
	state->alias_exp = (t_string){};
	state->alias_exp_owned = false;
	xfree(state->pid);
	xfree(state->last_bg_pid);
	xfree(state->ctx);
	xfree(state->dft_ctx);
	state->ctx = 0;
	state->dft_ctx = 0;
	xfree(state->rl.buff.ctx);
	if (state->path_dirs)
		free_tab(state->path_dirs);
	state->path_dirs = NULL;
	xfree(state->path_dirs_src);
	state->path_dirs_src = NULL;
}

/* Session data, still in free_all_state's order: history, cwd, positional
   args, the argv pool, the trap table (handler strings are ft_strdup'd in
   set_one_trap and live for the whole session — freeing them here keeps a
   script using trap leak-clean like any other), then the dirstack and the
   for-loop positional snapshot. */
static void	free_session_data(t_shell *state)
{
	int	i;

	free_hist(state);
	xfree(state->cwd.ctx);
	pos_free(&state->pos);
	free_argv_pool(state);
	i = -1;
	while (++i < SH_NTRAP)
	{
		xfree(state->traps[i]);
		state->traps[i] = NULL;
	}
	free_dirstack(state);
	if (state->for_snapshot)
		free_positional_snapshot(state->for_snapshot);
	state->for_snapshot = NULL;
}

/* The single canonical shutdown path. Order is deliberate: functions first
   (they may reference variables), then the owned strings and caches, then
   per-command scratch (redirects, proc subs, AST), then session data
   (history, cwd, positional args, the argv pool, traps). env_index_free()
   must come after all env lookups; word_slab_teardown() is last so any
   stray word_free() calls during the earlier steps still work.
   alloc_live_report() is the ft_malloc leak oracle -- it's a no-op on libc
   builds and a canary for the slab backend. */
void	free_all_state(t_shell *state)
{
	free_functions(&state->functions);
	free_session_strings(state);
	free_redirects(&state->redirects);
	cleanup_proc_subs(state);
	free_ast(&state->tree);
	arr_marks_clear(state);
	attr_clear(state);
	free_session_data(state);
	env_index_free();
	parena_destroy();
	word_slab_teardown();
	alloc_live_report();
}
