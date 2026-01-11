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
# include "env.h"

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
		close(c.fd);
		free(c.fname);
		i++;
	}
	free(v->ctx);
	v->ctx = NULL;
	v->len = 0;
	v->cap = 0;
	v->elem_size = sizeof(t_redir);
}

void	free_all_state(t_shell *state)
{
	free(state->input.ctx);
	state->input = (t_string){};
	free(state->last_cmd_status_s);
	free(state->pid);
	free(state->context);
	free(state->base_context);
	state->context = 0;
	state->base_context = 0;
	free(state->readline_buff.buff.ctx);
	free_redirects(&state->redirects);
	free_ast(&state->tree);
	free_hist(state);
	free(state->cwd.ctx);
}

void	free_executable_cmd(t_executable_cmd cmd)
{
	size_t	i;

	i = 0;
	while (i < cmd.pre_assigns.len)
	{
		t_env *e = &((t_env *)cmd.pre_assigns.ctx)[i];
		free(e->value);
		free(e->key);
		i++;
	}
	i = 0;
	while (i < cmd.argv.len)
	{
		free(((char **)cmd.argv.ctx)[i]);
		i++;
	}
	free(cmd.pre_assigns.ctx);
	free(cmd.argv.ctx);
}

void	free_executable_node(t_executable_node *node)
{
	/* free redirs buffer (t_vec of ints) and reset vector */
	free(node->redirs.ctx);
	vec_init(&node->redirs);
}
