/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   execute_pipeline2.c                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/10 05:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/10 05:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "execution_private.h"

/* Run a pipeline that has exactly ONE stage and no leading `!`.  This is
   the overwhelmingly common shape — every plain command in a script or
   loop body — and the multi-stage scaffolding (results vector, per-stage
   template copy via the iteration context, pipe wiring, finalize pass,
   status harvest) is pure overhead for it.  The semantics mirror the
   n==1 trip through execute_pipeline_children exactly: same std-fd
   passthrough (non-std fds are dup'd into an owned copy and closed
   here), same modify_parent_ctx (idx 0 IS the last index), and the same
   wait-if-forked status resolution (for a single element, pipefail and
   take-the-last agree by construction). */
t_execution_state	execute_pipeline_one(t_shell *state,
						t_executable_node *exe)
{
	t_executable_node	curr;
	t_execution_state	res;

	curr = *exe;
	vec_init(&curr.redirs);
	curr.redirs.elem_size = sizeof(int);
	curr.next_infd = -1;
	if (exe->infd != STDIN_FILENO)
		curr.infd = dup(exe->infd);
	if (exe->outfd != STDOUT_FILENO)
		curr.outfd = dup(exe->outfd);
	curr.node = vec_idx(&exe->node->children, 0);
	res = execute_command(state, &curr);
	if (curr.outfd >= 0 && curr.outfd != STDOUT_FILENO)
		close(curr.outfd);
	if (curr.infd >= 0 && curr.infd != STDIN_FILENO)
		close(curr.infd);
	free_executable_node(state, &curr);
	procsub_close_fds_parent(state);
	if (res.pid != -1)
		exe_res_set_status(state, &res);
	jc_end(state);
	set_pipestatus_one(state, res.status);
	return (res);
}

/* A leading `!` flips the status between 0 and 1 and clears the pid so
   callers treat it as an immediate status, not a background job.
   PIPESTATUS keeps the command's own status: bash records `! false` as 1
   there and answers 0 to $?.  A command that is UNWINDING -- `! return 1`,
   `! exit 3`, `! break` -- keeps its status untouched: bash's return and
   exit leave before the negation is applied, so `f() { ! return 1; }`
   returns 1, not 0. */
t_execution_state	negate_status(t_shell *state, t_execution_state res)
{
	if (state->should_exit || state->func_return || state->loop_break
		|| state->loop_continue)
		return (res);
	res.status = !res.status;
	res.pid = -1;
	return (res);
}

/* The one-stage pipeline, negated or not.  set -e is suspended for the
   whole run of a negated command, function body included, the same way
   it is inside an if or while condition (errexit_off): bash goes on
   after `set -e; f() { false; }; ! f`. */
t_execution_state	pipeline_single(t_shell *state, t_executable_node *exe)
{
	t_execution_state	res;

	state->errexit_off += exe->node->negate;
	res = execute_pipeline_one(state, exe);
	state->errexit_off -= exe->node->negate;
	if (exe->node->negate)
		return (negate_status(state, res));
	return (res);
}
