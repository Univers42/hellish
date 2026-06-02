/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   execute_command.c                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 15:08:41 by dlesieur          #+#    #+#             */
/*   Updated: 2026/03/15 01:30:03 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "execution_private.h"

static void	ensure_redirs_initialized(t_executable_node *exe);
static int	collect_redirects_from_ast(t_shell *state, t_executable_node *exe);

t_execution_state	execute_if(t_shell *state, t_executable_node *exe);
t_execution_state	execute_while(t_shell *state, t_executable_node *exe);
t_execution_state	execute_for(t_shell *state, t_executable_node *exe);
t_execution_state	execute_case(t_shell *state, t_executable_node *exe);
t_execution_state	execute_func_def(t_shell *state, t_executable_node *exe);

static t_execution_state	run_compound(t_shell *state,
								t_executable_node *exe)
{
	t_ast_type	t;

	t = exe->node->node_type;
	if (t == AST_IF)
		return (execute_if(state, exe));
	if (t == AST_WHILE || t == AST_UNTIL)
		return (execute_while(state, exe));
	if (t == AST_FOR)
		return (execute_for(state, exe));
	if (t == AST_CASE)
		return (execute_case(state, exe));
	if (t == AST_BRACE_GROUP)
		return (exe->node = vec_idx(&exe->node->children, 0),
			execute_tree_node(state, exe));
	return (ft_assert(0), res_status(1));
}

static t_execution_state	dispatch_compound(t_shell *state,
								t_executable_node *exe)
{
	t_execution_state	res;
	int					bak[3];
	int					pid;

	if (!exe->modify_parent_ctx)
	{
		pid = fork();
		if (pid == 0)
		{
			default_signal_handlers();
			set_up_redirection(state, exe);
			exit(run_compound(state, exe).status);
		}
		return (res_pid(pid));
	}
	bak[0] = dup(STDIN_FILENO);
	bak[1] = dup(STDOUT_FILENO);
	bak[2] = dup(STDERR_FILENO);
	set_up_redirection(state, exe);
	exe->infd = -1;
	exe->outfd = -1;
	exe->next_infd = -1;
	res = run_compound(state, exe);
	dup2(bak[0], STDIN_FILENO);
	dup2(bak[1], STDOUT_FILENO);
	dup2(bak[2], STDERR_FILENO);
	close(bak[0]);
	close(bak[1]);
	close(bak[2]);
	return (res);
}

t_execution_state	execute_command(t_shell *state, t_executable_node *exe)
{
	t_ast_type	ft;

	ft_assert(exe->node->children.len >= 1);
	ft = ((t_ast_node *)exe->node->children.ctx)[0].node_type;
	if (ft == AST_FUNCTION_DEF)
		return (exe->node = &((t_ast_node *)exe->node->children.ctx)[0],
			execute_func_def(state, exe));
	if (ft == AST_SIMPLE_COMMAND)
		return (exe->node = &((t_ast_node *)exe->node->children.ctx)[0],
			execute_simple_command(state, exe));
	ensure_redirs_initialized(exe);
	if (collect_redirects_from_ast(state, exe))
		return (res_status(AMBIGUOUS_REDIRECT));
	exe->node = vec_idx(&exe->node->children, 0);
	if (ft == AST_SUBSHELL)
		return (exe->modify_parent_ctx = true,
			execute_subshell(state, exe));
	return (dispatch_compound(state, exe));
}

/* Ensure exe->redirs is initialized */
static void	ensure_redirs_initialized(t_executable_node *exe)
{
	if (!exe->redirs.ctx)
	{
		vec_init(&exe->redirs);
		exe->redirs.elem_size = sizeof(int);
	}
}

/*
** Collect redirect indices from exe->node children (starting at index 1)
** Returns 0 on success, AMBIGUOUS_REDIRECT if
redirect_from_ast_redir signals error.
*/
static int	collect_redirects_from_ast(t_shell *state, t_executable_node *exe)
{
	size_t		i;
	t_ast_node	*curr;
	int			redir_idx;

	i = 0;
	while (++i < exe->node->children.len)
	{
		curr = vec_idx(&exe->node->children, i);
		ft_assert(curr->node_type == AST_REDIRECT);
		if (redirect_from_ast_redir(state, curr, &redir_idx))
			return (AMBIGUOUS_REDIRECT);
		vec_push_int(&exe->redirs, redir_idx);
	}
	return (0);
}
