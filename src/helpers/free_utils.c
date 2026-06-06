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
	env_index_free();
	word_slab_teardown();
	alloc_live_report();
}

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

void	free_executable_node(t_executable_node *node)
{
	xfree(node->redirs.ctx);
	vec_init(&node->redirs);
}
