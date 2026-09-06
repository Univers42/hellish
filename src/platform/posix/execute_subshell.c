/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   execute_subshell.c                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 15:08:21 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/27 16:31:36 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "execution_private.h"
#include "ft_builtins.h"

/* What a ( compound-list ) does once it is in its own process, whoever
   forked it.  Key points:
   - The EXIT trap (traps[0]) is cleared: it belongs to the parent shell,
     not the subshell; the child runs run_exit_trap at the end from its own
     (now-empty) table.
   - We call free_executable_node before execute_tree_node so the pipe fds
     (next_infd etc.) have already been closed; then we reset infd/outfd so
     set_up_redirection does not try to dup2 the now-closed values.
   - forward_exit_status encodes the child's exit code via _exit so the
     parent's waitpid gets the right WEXITSTATUS.  Never returns. */
static void	subshell_body(t_shell *state, t_executable_node *exe)
{
	t_execution_state	res;

	xfree(state->traps[0]);
	state->traps[0] = NULL;
	pseudo_traps_quiet(state);
	set_up_redirection(state, exe);
	exe->node = &((t_ast_node *)exe->node->children.ctx)[0];
	free_executable_node(state, exe);
	exe->outfd = 1;
	exe->infd = 0;
	res = execute_tree_node(state, exe);
	run_exit_trap(state);
	forward_exit_status(res);
}

/* The child half of a foreground ( compound-list ), right after the fork:
   - jc_child comes first: it must move us into the job's process group and
     take the terminal while SIGTTOU is still inherited as SIG_IGN.
   - set_unwind_sig installs the child's SIGINT handler so a ^C to the
     child group unwinds cleanly without a double-free.
   Neither belongs to an & child: that one already sits in its own group
   and must keep SIGINT ignored (async_child_signals).  Never returns. */
static void	subshell_child(t_shell *state, t_executable_node *exe)
{
	jc_child(state);
	set_unwind_sig();
	subshell_body(state, exe);
}

/* Run a ( compound-list ) in a forked child and hand back its pid -- unless
   we already ARE the child: a `( ... ) &` list forks once in bg_child_body,
   which marks this node as the one it may run in place (bg_lone_command).
   Forking again put the body, and any trap it sets, in a grandchild while
   $! named a middle process nobody had a handler in; `kill $!; wait $!`
   then reported 143 where bash reports the trap's exit, and the body kept
   running as an orphan.  bash forks once here, and so do we. */
t_execution_state	execute_subshell(t_shell *state, t_executable_node *exe)
{
	int	pid;

	if (state->bg_exec_node && state->bg_exec_node == exe->node)
		subshell_body(state, exe);
	pid = fork();
	if (pid == 0)
		subshell_child(state, exe);
	if (pid < 0)
		critical_error_errno_ctx("fork");
	jc_parent(state, pid);
	procsub_close_fds_parent(state);
	free_executable_node(state, exe);
	return (res_pid(pid));
}
