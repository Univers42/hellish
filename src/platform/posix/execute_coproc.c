/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   execute_coproc.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/22 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/07/22 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "execution_private.h"
#include "ft_builtins.h"

/* coproc [NAME] command: run `command` in a background child behind two
   pipes. The child's stdin reads what the parent writes to NAME[1], and
   its stdout is what the parent reads from NAME[0]; NAME_PID holds the
   pid. The AST_COPROC node carries NAME in its token (empty => COPROC)
   and the wrapped AST_COMMAND as child[0]. Field-level v1 scope: no
   trailing redirection list on the coproc itself. */

/* The coproc NAME as a fresh string: the node's token slice, or the
   default "COPROC" when no name was given. */
static char	*coproc_name(t_ast_node *node)
{
	if (node->token.start && node->token.len > 0)
		return (ft_strndup(node->token.start, node->token.len));
	return (ft_strdup("COPROC"));
}

/* Reset the traps a coprocess child must not inherit (bash without
   functrace/errtrace): the EXIT trap belongs to the parent, and DEBUG/
   RETURN/ERR would otherwise fire inside the child and leak their output
   into the pipe. trap_restore frees the current pseudo-traps and installs
   the blank set. */
static void	coproc_reset_traps(t_shell *state)
{
	char	*blank[3];

	blank[0] = NULL;
	blank[1] = NULL;
	blank[2] = NULL;
	xfree(state->traps[0]);
	state->traps[0] = NULL;
	trap_restore(state, blank);
}

/* Child side: wire stdin<-inp[0], stdout->outp[1], drop every pipe end,
   then run the wrapped command and _exit with its status (via the same
   forward_exit_status path subshells use). Own process group so the
   terminal's Ctrl-C does not reach the coprocess. */
static void	coproc_child(t_shell *state, t_executable_node *exe,
				int *inp, int *outp)
{
	t_execution_state	res;

	default_signal_handlers();
	setpgid(0, 0);
	coproc_reset_traps(state);
	dup2(inp[0], STDIN_FILENO);
	dup2(outp[1], STDOUT_FILENO);
	close(inp[0]);
	close(inp[1]);
	close(outp[0]);
	close(outp[1]);
	exe->node = (t_ast_node *)exe->node->children.ctx;
	exe->modify_parent_ctx = true;
	exe->infd = STDIN_FILENO;
	exe->outfd = STDOUT_FILENO;
	res = execute_command(state, exe);
	if (res.pid != -1)
		exe_res_set_status(state, &res);
	run_exit_trap(state);
	forward_exit_status(res);
}

/* Parent bookkeeping after the fork: keep the read/write ends (already
   moved to high CLOEXEC fds by the caller) recorded in NAME/NAME_PID,
   count the job so reap_background_children waits on it, and announce it
   interactively like a `&` background job. */
static void	coproc_parent(t_shell *state, t_ast_node *node, int *fds,
				pid_t pid)
{
	char	*name;
	t_job	*job;

	name = coproc_name(node);
	coproc_store(state, name, fds, pid);
	xfree(name);
	state->bg_job_count++;
	job = job_add(&state->job_table, pid, "coproc", true);
	if (state->metinp == INP_RL && job)
		ft_printf("[%d] %d\n", job->id, pid);
}

/* Set up the two pipes, fork, and hand off to child/parent. On any pipe
   or fork failure every fd opened so far is closed and status 1 returned.
   fds[] carries the parent's read (outp[0]) and write (inp[1]) ends,
   relocated above fd 9 with close-on-exec so unrelated children never
   inherit them (which would wedge the coprocess's EOF). */
t_execution_state	execute_coproc(t_shell *state, t_executable_node *exe)
{
	int		inp[2];
	int		outp[2];
	int		fds[2];
	pid_t	pid;

	if (pipe(outp) < 0)
		return (res_status(1));
	if (pipe(inp) < 0)
		return (close(outp[0]), close(outp[1]), res_status(1));
	pid = fork();
	if (pid == 0)
		coproc_child(state, exe, inp, outp);
	close(inp[0]);
	close(outp[1]);
	if (pid < 0)
		return (close(inp[1]), close(outp[0]), res_status(1));
	fds[0] = save_fd(outp[0]);
	fds[1] = save_fd(inp[1]);
	close(outp[0]);
	close(inp[1]);
	return (coproc_parent(state, exe->node, fds, pid), res_status(0));
}
