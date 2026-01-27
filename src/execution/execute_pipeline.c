/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   execute_pipeline.c                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 23:31:01 by marvin            #+#    #+#             */
/*   Updated: 2026/01/09 23:31:01 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "execution_private.h"

void	set_up_redir_pipeline_child(bool is_last, t_executable_node *exe,
									t_executable_node *curr_exe, int (*pp)[2])
{
	if (!is_last)
	{
		if (pipe(*pp))
			critical_error_errno_ctx("pipe");
		curr_exe->outfd = (*pp)[1];
		curr_exe->next_infd = (*pp)[0];
	}
	else
	{
		curr_exe->next_infd = -1;
		curr_exe->outfd = dup(exe->outfd);
	}
}

/* prepare per-child execution state (copy template,
init redirs, set fds/node, setup pipe) */
static void	prepare_child_exec(t_exec_child_ctx *c)
{
	size_t	last_index;

	*c->curr_exe = *c->exe;
	vec_init(&c->curr_exe->redirs);
	c->curr_exe->redirs.elem_size = sizeof(int);
	c->curr_exe->infd = c->prev_infd;
	last_index = c->last_index;
	c->curr_exe->modify_parent_ctx = (c->idx == last_index)
		&& c->exe->modify_parent_ctx;
	c->curr_exe->node = vec_idx(&c->exe->node->children, c->idx);
	ft_assert(c->curr_exe->node->node_type == AST_COMMAND);
	set_up_redir_pipeline_child(c->idx == last_index,
		c->exe, c->curr_exe, c->pp);
}

/* finalize parent-side after child execution: close fds
and set prev_infd for next child */
static void	finalize_child_parent(t_exec_child_ctx *c)
{
	if (c->curr_exe->outfd >= 0)
		close(c->curr_exe->outfd);
	if (c->curr_exe->infd >= 0)
		close(c->curr_exe->infd);
	if (c->idx == c->last_index)
		c->prev_infd = -1;
	else
		c->prev_infd = (*c->pp)[0];
}

void	execute_pipeline_children(t_shell *state,
								t_executable_node *exe,
								t_vec_exe_res *results)
{
	t_executable_node	curr_exe;
	int					pp[2];
	t_exec_child_ctx	ctx;
	t_execution_state	res;

	ctx.prev_infd = dup(exe->infd);
	ctx.state = state;
	ctx.exe = exe;
	ctx.curr_exe = &curr_exe;
	ctx.pp = &pp;
	if (exe->node->children.len == 0)
		ctx.last_index = 0;
	else
		ctx.last_index = exe->node->children.len - 1;
	ctx.results = results;
	ctx.idx = 0;
	while (ctx.idx < exe->node->children.len)
	{
		prepare_child_exec(&ctx);
		res = execute_command(state, &curr_exe);
		vec_push(results, &res);
		finalize_child_parent(&ctx);
		ctx.idx++;
	}
}

/* Always returns status */
t_execution_state	execute_pipeline(t_shell *state, t_executable_node *exe)
{
	t_vec_exe_res		results;
	t_execution_state	res;
	t_execution_state	*plast;

	results = (t_vec_exe_res){0};
	vec_init(&results);
	results.elem_size = sizeof(t_execution_state);
	execute_pipeline_children(state, exe, &results);
	procsub_close_fds_parent(state);
	if (results.len > 0)
	{
		plast = (t_execution_state *)vec_idx(&results, results.len - 1);
		res = *plast;
		if (res.pid != -1)
			exe_res_set_status(&res);
	}
	else
		res = res_status(0);
	free(results.ctx);
	return (res);
}
