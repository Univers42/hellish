/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   execute_pipeline.c                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 23:31:01 by marvin            #+#    #+#             */
/*   Updated: 2026/01/09 23:31:01 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "execution_private.h"

/* Wire up pipe fds for one element of a pipeline.  Non-last elements get a
   fresh pipe: write end goes to curr_exe->outfd (child will dup2 it to
   stdout), read end lands in curr_exe->next_infd (the PARENT saves it as
   prev_infd so the NEXT child picks it up as its stdin).  The last element
   inherits the pipeline's own outfd: stdout (fd 1) passes through AS IS --
   nothing downstream ever closes fd 1, and handing the stage a dup would
   make fd_setup_needed() true for every plain command, forcing the full
   backup/redirect/restore dance (15 fd syscalls per command in a hot loop).
   Only a non-standard outfd (nested/redirected pipeline) is dup'd so this
   stage owns a closable copy. */
void	set_up_redir_pipeline_child(bool is_last, t_executable_node *exe,
									t_executable_node *curr_exe, int (*pp)[2])
{
	if (!is_last)
	{
		if (pal_pipe(*pp))
			critical_error_errno_ctx("pipe");
		curr_exe->outfd = (*pp)[1];
		curr_exe->next_infd = (*pp)[0];
	}
	else
	{
		curr_exe->next_infd = -1;
		curr_exe->outfd = exe->outfd;
		if (exe->outfd != STDOUT_FILENO)
			curr_exe->outfd = dup(exe->outfd);
	}
}

/* Stamp out a fresh t_executable_node for one pipeline element.  We copy
   the template (inheriting modify_parent_ctx, redirs, etc.) then zero the
   redirs vec so each child builds its own redirect list independently.
   The modify_parent_ctx flag is only kept for the last stage -- inner
   stages always fork, so they can't modify parent state anyway. */
static void	prepare_child_exec(t_exec_child_ctx *c)
{
	size_t	last_index;

	*c->curr_exe = *c->exe;
	vec_init(&c->curr_exe->redirs);
	c->curr_exe->redirs.elem_size = sizeof(int);
	c->curr_exe->infd = c->prev_infd;
	last_index = c->last_index;
	c->curr_exe->modify_parent_ctx = false;
	c->curr_exe->node = vec_idx(&c->exe->node->children, c->idx);
	ft_assert(c->curr_exe->node->node_type == AST_COMMAND);
	set_up_redir_pipeline_child(c->idx == last_index,
		c->exe, c->curr_exe, c->pp);
}

/* Parent-side cleanup after dispatching one pipeline stage.  We close
   the write end (outfd) and the now-consumed read end (infd) of the pipe
   from the perspective of the parent -- if we left them open the child
   reading from the next pipe stage would never see EOF.  Then we slide
   the new pipe read end (pp[0]) forward as prev_infd for the next child
   to inherit as its stdin.  free_executable_node drops any redirect
   vec memory that belongs to this stage. */
static void	finalize_child_parent(t_exec_child_ctx *c)
{
	if (c->curr_exe->outfd >= 0 && c->curr_exe->outfd != STDOUT_FILENO)
		close(c->curr_exe->outfd);
	if (c->curr_exe->infd >= 0 && c->curr_exe->infd != STDIN_FILENO)
		close(c->curr_exe->infd);
	if (c->idx == c->last_index)
		c->prev_infd = -1;
	else
		c->prev_infd = (*c->pp)[0];
	free_executable_node(c->curr_exe);
}

/* Drive the pipeline loop: one iteration per AST_COMMAND child.  Each
   child is prepared (pipe wiring), dispatched via execute_command (which
   may fork or run inline if modify_parent_ctx), then finalized in the
   parent (fd cleanup, prev_infd advance).  Results (pid or immediate
   status) accumulate in `results` for pipeline_status to harvest. */
void	execute_pipeline_children(t_shell *state,
								t_executable_node *exe,
								t_vec_exe_res *results)
{
	t_executable_node	curr_exe;
	int					pp[2];
	t_exec_child_ctx	ctx;
	t_execution_state	res;

	ctx = (t_exec_child_ctx){.state = state, .exe = exe,
		.curr_exe = &curr_exe, .pp = &pp, .results = results,
		.prev_infd = exe->infd, .idx = 0, .last_index = 0};
	if (exe->infd != STDIN_FILENO)
		ctx.prev_infd = dup(exe->infd);
	if (exe->node->children.len > 0)
		ctx.last_index = exe->node->children.len - 1;
	while (ctx.idx < exe->node->children.len)
	{
		prepare_child_exec(&ctx);
		res = execute_command(state, &curr_exe);
		vec_push(results, &res);
		finalize_child_parent(&ctx);
		ctx.idx++;
	}
}

/* Top-level pipeline executor.  Runs all children, closes any lingering
   process-substitution fds in the parent (they were opened for the
   children and would otherwise never close), then collects the exit
   status.  A leading `!` (node->negate) flips the status between 0 and
   1 and clears the pid so callers treat it as an immediate status not a
   background job.  pipefail vs last-stage logic lives in pipeline_status.
   The results vec is freed here -- each element was either already waited
   (pipefail_scan) or is a pid that pipeline_status just waitpid'd. */
t_execution_state	execute_pipeline(t_shell *state, t_executable_node *exe)
{
	t_vec_exe_res		results;
	t_execution_state	res;

	jc_begin(state);
	if (exe->node->children.len == 1 && !exe->node->negate)
		return (execute_pipeline_one(state, exe));
	results = (t_vec_exe_res){0};
	vec_init(&results);
	results.elem_size = sizeof(t_execution_state);
	execute_pipeline_children(state, exe, &results);
	procsub_close_fds_parent(state);
	if (results.len > 0)
		res = pipeline_status(state, &results);
	else
		res = res_status(0);
	jc_end(state);
	if (exe->node->negate)
	{
		res.status = !res.status;
		res.pid = -1;
	}
	xfree(results.ctx);
	return (res);
}
