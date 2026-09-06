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

/* The child half of a ( compound-list ).  Split out of execute_subshell so
   both halves stay inside the line ceiling once job control is wired in.
   Key points:
   - jc_child comes first: it must move us into the job's process group and
     take the terminal while SIGTTOU is still inherited as SIG_IGN.
   - set_unwind_sig installs the child's SIGTERM handler so a kill to the
     child group unwinds cleanly without a double-free.
   - The EXIT trap (traps[0]) is cleared: it belongs to the parent shell,
     not the subshell; the child runs run_exit_trap at the end from its own
     (now-empty) table.
   - We call free_executable_node before execute_tree_node so the pipe fds
     (next_infd etc.) have already been closed; then we reset infd/outfd so
     set_up_redirection does not try to dup2 the now-closed values.
   - forward_exit_status encodes the child's exit code via _exit so the
     parent's waitpid gets the right WEXITSTATUS.  Never returns. */
static void	subshell_child(t_shell *state, t_executable_node *exe)
{
	t_execution_state	res;

	jc_child(state);
	set_unwind_sig();
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

/* Run a ( compound-list ) in a forked child and hand back its pid. */
t_execution_state	execute_subshell(t_shell *state, t_executable_node *exe)
{
	int	pid;

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
