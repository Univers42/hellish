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
#include "sh_input.h"

void	exit_clean(t_shell *state, int code);

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

	reparse_all(state, ast);
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

/* An error that must not let the next command run with a variable in a
** state the script never asked for -- a readonly assignment, a subscript
** that names no element.  HOW FAR it unwinds is MEASURED, not assumed:
**
**   -c 'readonly r=1; r=2; echo R'   bash and zsh both abort, rc 1
**   inside a sourced file            zsh abandons the FILE (source returns
**                                    126), bash abandons only the rest of
**                                    that command list -- NEITHER kills the
**                                    shell
**   interactive                      status only, keep going
**
** So inside a sourced file we unwind the file the way `return` does --
** func_return, which exec_string_inner clears at the file boundary -- and
** only a top-level script or -c actually exits.
**
** Killing the shell from inside a sourced file is what took oh-my-zsh's git
** plugin from 201 aliases to nothing: four lines from the end it writes
** `aliases[$name]=`, `aliases` is a zsh special we do not have, so the
** subscript is invalid and the whole `-c 'source ...'` died before anything
** could look at what the plugin had defined.
*/
void	fatal_input_error(t_shell *state, int code)
{
	if (state->source_depth > 0)
		state->func_return = 1;
	else if (state->metinp != INP_RL)
		exit_clean(state, code);
}
