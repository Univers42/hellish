/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   executor.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 23:30:41 by marvin            #+#    #+#             */
/*   Updated: 2026/01/09 23:30:41 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "shell.h"
#include "../heredoc/redir.h"

#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include "../libft/libft.h"

#include <readline/readline.h>
#include <string.h>
#include <unistd.h>

#include "../ft_printf/ft_printf.h"

// returns pid
t_exe_res	execute_subshell(t_shell *state, t_executable_node *exe)
{
	int			pid;
	t_exe_res	res;

	pid = fork();
	if (pid == 0)
	{
		set_unwind_sig();
		set_up_redirection(state, exe);
		exe->node = &((t_ast_node *)exe->node->children.ctx)[0];
		free_executable_node(exe);
		exe->outfd = 1;
		exe->infd = 0;
		res = execute_tree_node(state, exe);
		forward_exit_status(res);
	}
	if (pid < 0)
		critical_error_errno_context("fork");
	free_executable_node(exe);
	return (res_pid(pid));
}

// gives back pid;
t_exe_res	execute_command(t_shell *state, t_executable_node *exe)
{
	t_ast_node	*curr;
	size_t		i;
	int			redir_idx;

	ft_assert(exe->node->children.len >= 1);
	if (((t_ast_node *)exe->node->children.ctx)[0].node_type == AST_SIMPLE_COMMAND)
	{
		exe->node = &((t_ast_node *)exe->node->children.ctx)[0];
		return (execute_simple_command(state, exe));
	}
	ft_assert(((t_ast_node *)exe->node->children.ctx)[0].node_type == AST_SUBSHELL);
	/* ensure redirs vector initialized */
	if (!exe->redirs.ctx)
	{
		vec_init(&exe->redirs);
		exe->redirs.elem_size = sizeof(int);
	}
	i = 0;
	while (++i < exe->node->children.len)
	{
		curr = vec_idx(&exe->node->children, i);
		ft_assert(curr->node_type == AST_REDIRECT);
		if (redirect_from_ast_redir(state, curr, &redir_idx))
			return (res_status(AMBIGUOUS_REDIRECT));
		{ int idx = redir_idx; vec_push(&exe->redirs, &idx); }
	}
	exe->node = vec_idx(&exe->node->children, 0);
	exe->modify_parent_context = true;
	return (execute_subshell(state, exe));
}

// always returns status
t_exe_res	execute_tree_node(t_shell *state, t_executable_node *exe)
{
	t_exe_res		status;
	t_ast_t			t;

	t = exe->node->node_type;
	status = res_status(0);
	if (t == AST_COMMAND_PIPELINE)
		status = execute_pipeline(state, exe);
	else if (t == AST_SIMPLE_LIST || t == AST_COMPOUND_LIST)
		status = execute_simple_list(state, exe);
	else
		ft_assert("Unreachable" == 0);
	set_cmd_status(state, status);
	return (status);
}

void	execute_top_level(t_shell *state)
{
	t_executable_node	exe;
	t_exe_res			res;

	exe = (t_executable_node){.infd = 0, .outfd = 1, .node = &state->tree,
		.modify_parent_context = true};
	/* initialize redirs vector for this execution node */
	vec_init(&exe.redirs);
	exe.redirs.elem_size = sizeof(int);
	state->heredoc_idx = 0;
	if (!g_should_unwind)
		gather_heredocs(state, &state->tree);
	if (!g_should_unwind)
		res = execute_tree_node(state, &exe);
	else
		res = res_status(CANCELED);
	if (res.c_c)
	{
		if (state->input_method == INP_READLINE)
			ft_eprintf("\n");
		else
			state->should_exit = true;
	}
	state->last_cmd_status_res = res;
}
