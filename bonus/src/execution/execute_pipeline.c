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

#include "../shell.h"
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

void	execute_pipeline_children(t_state *state, t_executable_node *exe,
	t_vec_exe_res *results)
{
	size_t				i;
	t_executable_node	curr_exe;
	int					pp[2];

	i = 0;
	curr_exe = (t_executable_node){};
	curr_exe.infd = dup(exe->infd);
	curr_exe.modify_parent_context = exe->node->children.len == 1;
	while (i < exe->node->children.len)
	{
		curr_exe.node = vec_idx(&exe->node->children, i);
		ft_assert(curr_exe.node->node_type == AST_COMMAND);
		set_up_redir_pipeline_child(i == exe->node->children.len - 1,
			exe, &curr_exe, &pp);
		/* execute_command returns a t_exe_res; push its copy into results */
		{
			t_exe_res r = execute_command(state, &curr_exe);
			vec_push(results, &r);
		}
		close(curr_exe.outfd);
		close(curr_exe.infd);
		curr_exe.infd = pp[0];
		i++;
	}
}

// Always returns status
t_exe_res	execute_pipeline(t_state *state, t_executable_node *exe)
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
