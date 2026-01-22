/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   execute_subshell.c                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 15:08:21 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/22 15:08:31 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "execution_private.h"

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