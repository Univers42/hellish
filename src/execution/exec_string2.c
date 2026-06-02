/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   exec_string2.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/02 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/02 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "execution_private.h"
#include "decomposer.h"
#include "redir.h"

int	run_parsed(t_shell *state, t_ast_node *ast)
{
	t_executable_node	exe;
	t_execution_state	res;

	reparse_words(ast);
	reparse_assignment_words(ast);
	exe = create_exe_node(0, 1, ast, true);
	vec_init(&exe.redirs);
	exe.redirs.elem_size = sizeof(int);
	gather_heredocs(state, ast, false);
	res = execute_tree_node(state, &exe);
	set_cmd_status(state, res);
	return (res.status);
}

bool	must_stop(t_shell *state)
{
	return (state->should_exit || state->func_return || state->loop_break
		|| state->loop_continue || get_g_sig()->should_unwind);
}
