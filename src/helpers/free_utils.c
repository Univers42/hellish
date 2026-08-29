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
#include "sh_alias.h"
#include "cmd_hash.h"
#include "zle.h"

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
   whole body AST. Then the bodies unset mid-call and parked by retire_body
   (func_retire.c) -- at shutdown no call can still be walking one, so those
   are dropped unconditionally rather than through drain_dead_funcs'
   func_depth guard, which a shell exiting from inside a function would
   never satisfy. Each vec's backing array is freed last, same pattern as
   every other vec-of-structs we own. Called once from free_all_state. */
static void	free_functions(t_shell *state)
{
	size_t			i;
	t_shell_func	*fn;

	i = 0;
	while (i < state->functions.len)
	{
		fn = vec_idx(&state->functions, i++);
		xfree(fn->name);
		xfree(fn->src);
		xfree(fn->text);
		free_ast(&fn->body);
	}
	hash_destroy(&state->func_index, NULL);
	xfree(state->functions.ctx);
	i = 0;
	while (i < state->dead_funcs.len)
		free_ast((t_ast_node *)vec_idx(&state->dead_funcs, i++));
	xfree(state->dead_funcs.ctx);
	i = 0;
	while (i < state->call_frames.len)
	{
		xfree(((t_call_frame *)vec_idx(&state->call_frames, i))->func);
		xfree(((t_call_frame *)vec_idx(&state->call_frames, i++))->src);
	}
	xfree(state->call_frames.ctx);
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

/* Session data, still in free_all_state's order: history, aliases, cwd,
   positional args, the argv pool, the trap table (handler strings are
   ft_strdup'd in set_one_trap and live for the whole session — freeing them
   here keeps a script using trap leak-clean like any other), then the
   dirstack and the for-loop positional snapshot.

   alias_table_free existed and was correct and was never CALLED, so every
   alias leaked its name, its value and its entry. It went unnoticed for two
   reasons that reinforce each other: our own tests define a handful of
   aliases, and LeakSanitizer does not report the table at all — it is still
   reachable through state at exit, which LSan classes as "still reachable"
   rather than leaked. The ft_malloc oracle counts live bytes instead of
   reachability, so it sees it; sourcing oh-my-zsh's git plugin, which
   defines 201 aliases, put 18 KB on the board. Exactly the case the SAFE=0
   parity run exists for.

   cmd_hash_free was the same shape and found by the same run: a correct
   destructor that only `hash -r` ever called. It matters more than the alias
   one looks like it should, because prehash_external populates that cache
   on every external command a session runs -- ~88 bytes each, forever.

   The ZLE tables are here for the same reason and were caught the same way:
   200 `zle -N` registrations put 17 KB on the ft_malloc oracle. They live in
   function-local statics rather than in t_shell -- readline's callback takes
   no context, so they have to be reachable without one -- which is exactly
   the shape that gets forgotten at shutdown. */
static void	free_session_data(t_shell *state)
{
	int	i;

	free_hist(state);
	alias_table_free(&state->aliases);
	cmd_hash_free(&state->cmd_cache);
	zle_widgets_free();
	zle_binds_free();
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
	free_functions(state);
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
