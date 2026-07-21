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
	free_executable_node(&curr);
	procsub_close_fds_parent(state);
	if (res.pid != -1)
		exe_res_set_status(&res);
	set_pipestatus_one(state, res.status);
	return (res);
}
