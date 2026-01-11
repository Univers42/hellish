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

#include "shell.h"
#include <unistd.h>
#include "../libft/libft.h"

void	set_up_redir_pipeline_child(bool is_last, t_executable_node *exe,
	t_executable_node *curr_exe, int (*pp)[2])
{
	if (!is_last)
	{
		if (pipe(*pp))
			critical_error_errno_context("pipe");
		curr_exe->outfd = (*pp)[1];
		curr_exe->next_infd = (*pp)[0];
	}
	else
	{
		curr_exe->next_infd = -1;
		curr_exe->outfd = dup(exe->outfd);
	}
}

void	execute_pipeline_children(t_shell *state, t_executable_node *exe,
	t_vec_exe_res *results)
{
	size_t				i;
	t_executable_node	curr_exe;
	int					pp[2];
	int					prev_infd;

	/* start input for first child from exe->infd (dup so parent can close safely) */
	prev_infd = dup(exe->infd);
	i = 0;
	while (i < exe->node->children.len)
	{
		/* copy template and set per-child fields */
		curr_exe = *exe;
		/* fresh redirs vector for the child */
		vec_init(&curr_exe.redirs);
		curr_exe.redirs.elem_size = sizeof(int);
		/* set child's input from preserved fd */
		curr_exe.infd = prev_infd;
		/* only the last child may modify parent context (when the pipeline
		   as a whole was allowed to modify parent) */
		curr_exe.modify_parent_context = (i == exe->node->children.len - 1)
			&& exe->modify_parent_context;

		curr_exe.node = vec_idx(&exe->node->children, i);
		ft_assert(curr_exe.node->node_type == AST_COMMAND);

		/* set up pipe for this child (if not last) */
		set_up_redir_pipeline_child(i == exe->node->children.len - 1,
			exe, &curr_exe, &pp);

		/* execute_command returns a t_exe_res; push its copy into results */
		{
			t_exe_res r = execute_command(state, &curr_exe);
			vec_push(results, &r);
		}

		/* In parent: close the child's outfd (writer end) and its infd (the
		   read end we passed as input), then prepare prev_infd for next child */
		if (curr_exe.outfd >= 0)
			close(curr_exe.outfd);
		if (curr_exe.infd >= 0)
			close(curr_exe.infd);

		if (i == exe->node->children.len - 1)
			prev_infd = -1;
		else
			prev_infd = pp[0];

		i++;
	}
}

// Always returns status
t_exe_res	execute_pipeline(t_shell *state, t_executable_node *exe)
{
	t_vec_exe_res		results;
	t_exe_res			res;

	results = (t_vec_exe_res){0};
	vec_init(&results);
	results.elem_size = sizeof(t_exe_res);
	execute_pipeline_children(state, exe, &results);
	/* Use the exit status of the last command in the pipeline */
	if (results.len > 0)
	{
		t_exe_res *plast = (t_exe_res *)vec_idx(&results, results.len - 1);
		exe_res_set_status(plast);
		res = *plast;
	}
	else
		res = res_status(0);
	free(results.ctx);
	return (res);
}
