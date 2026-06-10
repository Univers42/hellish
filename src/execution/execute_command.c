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

static int	collect_redirects_from_ast(t_shell *state, t_executable_node *exe);

/* Dispatch a compound command (if/while/for/case/brace-group) that has
   already been identified.  Brace groups { } run their single child in
   place; every other type has a dedicated executor.  ft_assert(0) is the
   "should never happen" guard -- if we forgot a node type, crash loudly
   rather than silently returning 0. */
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

/* Run a compound command IN THE PARENT process (needed when it can modify
   parent state -- e.g. `{ cd /; }` must change the parent's cwd).  We
   save stdin/stdout/stderr with dup, redirect per the node, run, then
   restore via restore_fds.  The whole save/redirect/restore dance is
   skipped when fd_setup_needed says there is no fd work (no redirects,
   std fds) -- an `if` or `case` in a hot loop would otherwise pay 9 fd
   syscalls per execution for nothing. */
static t_execution_state	compound_in_parent(t_shell *state,
								t_executable_node *exe)
{
	t_execution_state	res;
	int					bak[3];
	int					need;

	need = fd_setup_needed(exe);
	if (need)
	{
		bak[0] = dup(STDIN_FILENO);
		bak[1] = dup(STDOUT_FILENO);
		bak[2] = dup(STDERR_FILENO);
		set_up_redirection(state, exe);
	}
	exe->infd = -1;
	exe->outfd = -1;
	exe->next_infd = -1;
	res = run_compound(state, exe);
	if (need)
		restore_fds(bak);
	return (res);
}

/* Decide whether a compound command forks or runs in the parent.  A
   subshell ( ... ) always forks (handled earlier).  Anything else forks
   UNLESS modify_parent_ctx is set -- that flag is true when we're the
   last (or only) command in a pipeline that feeds the parent's stdout,
   so redirections and variable changes must survive the call.  The child
   resets signal handlers to defaults (they were set to SIG_IGN for job
   control) before running. */
static t_execution_state	dispatch_compound(t_shell *state,
								t_executable_node *exe)
{
	int	pid;

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
	return (compound_in_parent(state, exe));
}

/* Central dispatch for an AST_COMMAND node.  The command node's first
   child tells us what kind of command it is: function definition, simple
   command, or a compound.  Simple commands and function defs are handed
   off immediately (they manage their own redirects).  Compounds need
   their redirect list built first from the sibling AST_REDIRECT children
   (children[1..]), then are routed to dispatch_compound which decides
   fork-vs-parent.  Subshells are a special case: they always need a fork
   but their modify_parent_ctx=true forces compound_in_parent path that
   handles the fd save/restore before forking. */
t_execution_state	execute_command(t_shell *state, t_executable_node *exe)
{
	t_ast_type	ft;

	if (state->opt_noexec && state->metinp != INP_RL)
		return (res_status(0));
	ft_assert(exe->node->children.len >= 1);
	ft = ((t_ast_node *)exe->node->children.ctx)[0].node_type;
	if (ft == AST_FUNCTION_DEF)
		return (exe->node = &((t_ast_node *)exe->node->children.ctx)[0],
			execute_func_def(state, exe));
	if (ft == AST_SIMPLE_COMMAND)
		return (exe->node = &((t_ast_node *)exe->node->children.ctx)[0],
			execute_simple_command(state, exe));
	if (!exe->redirs.ctx)
	{
		vec_init(&exe->redirs);
		exe->redirs.elem_size = sizeof(int);
	}
	if (collect_redirects_from_ast(state, exe))
		return (res_status(AMBIGUOUS_REDIRECT));
	exe->node = vec_idx(&exe->node->children, 0);
	if (ft == AST_SUBSHELL)
		return (exe->modify_parent_ctx = true,
			execute_subshell(state, exe));
	return (dispatch_compound(state, exe));
}

/* Walk the AST_COMMAND's children from index 1 onward (index 0 is the
   actual command); every child must be an AST_REDIRECT.  We resolve each
   redirect to a concrete t_redir (file open, dup target, heredoc fd) via
   redirect_from_ast_redir and push its index into exe->redirs.  The
   indices rather than the t_redir structs are stored so the redirect
   table lives in one place (state->redirects) and children can share an
   entry if the same redirect appears twice. */
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
