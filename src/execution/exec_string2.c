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

/* Execute a freshly-parsed AST tree.  We must reparse_words and
   reparse_assignment_words here because exec_string feeds a raw string
   that was tokenised without the full parser context (so assignment words
   like `FOO=bar` are plain TT_WORD tokens and need a second pass to be
   recognised as assignments).  gather_heredocs fills heredoc temp files
   before any child forks.  The result updates $? via set_cmd_status. */
int	run_parsed(t_shell *state, t_ast_node *ast)
{
	t_executable_node	exe;
	t_execution_state	res;

	reparse_all(ast);
	exe = create_exe_node(0, 1, ast, true);
	vec_init(&exe.redirs);
	exe.redirs.elem_size = sizeof(int);
	gather_heredocs(state, ast, false);
	res = execute_tree_node(state, &exe);
	set_cmd_status(state, res);
	return (res.status);
}

/* True when the shell must abort the current exec_string loop: explicit
   exit, return/break/continue escaping out of a function/loop body, or a
   signal-driven unwind.  Checked after each statement so we do not run
   the next command after `exit` or after Ctrl-C. */
bool	must_stop(t_shell *state)
{
	return (state->should_exit || state->func_return || state->loop_break
		|| state->loop_continue || get_g_sig()->should_unwind);
}
