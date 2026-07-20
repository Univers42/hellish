/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   execute_simple_list.c                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 23:30:47 by marvin            #+#    #+#             */
/*   Updated: 2026/01/09 23:30:47 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "execution_private.h"

void	run_pending_traps(t_shell *state);

/* Reap any zombie background children (non-blocking). Skip the waitpid syscall
   entirely when no background job has ever been launched (the common case for
   scripts and tight loops) — bg_job_count stays 0 until the first `&`. */
void	reap_background_children(t_shell *state)
{
	int		status;
	pid_t	pid;

	if (state->bg_job_count == 0)
		return ;
	pid = waitpid(-1, &status, WNOHANG);
	while (pid > 0)
	{
		bg_done_record(state, pid, status);
		pid = waitpid(-1, &status, WNOHANG);
	}
}

/* Decide whether to run the next command based on the operator that
   separates it from the previous one.  ; and newline always run.  &&
   short-circuits on failure; || short-circuits on success.  Ctrl-C
   (ctrl_c=true) stops execution regardless of operator so an interrupted
   pipeline does not stumble forward into the right-hand side. */
bool	should_execute(t_execution_state prev_status, t_tt prev_op)
{
	if (prev_status.ctrl_c)
		return (false);
	ft_assert(prev_status.status != -1);
	ft_assert(prev_op == TT_SEMICOLON || prev_op == TT_NEWLINE
		|| prev_op == TT_AND || prev_op == TT_OR || prev_op == TT_AMPERSAND);
	if (prev_op == TT_SEMICOLON || prev_op == TT_NEWLINE
		|| prev_op == TT_AMPERSAND)
		return (true);
	if (prev_op == TT_AND && prev_status.status == 0)
		return (true);
	if (prev_op == TT_OR && prev_status.status != 0)
		return (true);
	return (false);
}

/* Find the next & or ; operator in the children list starting from index i.
   Returns the index of the operator, or children.len if not found.
   Also sets *found_amp to true if & was found before ; */
size_t	find_next_separator(t_ast_node *node, size_t start, bool *found_amp)
{
	size_t		i;
	t_ast_node	*child;

	*found_amp = false;
	i = start;
	while (i < node->children.len)
	{
		child = &((t_ast_node *)node->children.ctx)[i];
		if (child->node_type == AST_TOKEN)
		{
			if (child->token.tt == TT_AMPERSAND)
			{
				*found_amp = true;
				return (i);
			}
			if (child->token.tt == TT_SEMICOLON
				|| child->token.tt == TT_NEWLINE)
				return (i);
		}
		i++;
	}
	return (node->children.len);
}

/* Deferred heredoc gathering for one range of a top-level list: fill the
   heredoc temp files of children [start, end) right before that range
   runs. Batched input can put a state-changing command (function def,
   assignment) and a later heredoc consumer in the same tree; gathering
   per range instead of per tree keeps bash's ordering — a heredoc body
   like `$(f)` must expand AFTER the earlier commands have executed. */
static void	gather_range_heredocs(t_shell *state, t_ast_node *node,
									size_t start, size_t end)
{
	t_ast_node	*kids;

	kids = (t_ast_node *)node->children.ctx;
	while (start < end)
	{
		if (!get_g_sig()->should_unwind)
			gather_heredocs(state, &kids[start], false);
		start++;
	}
}

/* Run an AST_SIMPLE_LIST or AST_COMPOUND_LIST.  These are sequences of
   pipelines separated by ; & newline && ||.  We scan forward from the
   current index to the next separator, execute the range as a foreground
   or background group, then run any pending traps (POSIX requires traps
   to fire after each simple command completes).  Background ranges (&)
   fork and return immediately; exit/break/continue/return/signal-unwind
   all break the outer loop so the shell does not run commands after an
   unconditional exit.  hd_defer is consumed exactly once, by the
   top-level list — nested lists were already gathered with their range. */
t_execution_state	execute_simple_list(t_shell *state, t_executable_node *exe)
{
	t_execution_state	status;
	size_t				i;
	size_t				sep_idx;
	bool				is_background;
	bool				defer;

	defer = state->hd_defer;
	state->hd_defer = false;
	reap_background_children(state);
	status = res_status(0);
	i = 0;
	while (i < exe->node->children.len)
	{
		sep_idx = find_next_separator(exe->node, i, &is_background);
		if (defer)
			gather_range_heredocs(state, exe->node, i, sep_idx);
		if (is_background)
			status = execute_range_background(state, exe, i, sep_idx);
		else
			status = execute_range(state, exe, i, sep_idx);
		i = sep_idx + 1;
		run_pending_traps(state);
		if (state->should_exit || state->loop_break || state->loop_continue
			|| state->func_return || get_g_sig()->should_unwind)
			break ;
	}
	reap_background_children(state);
	return (status);
}
