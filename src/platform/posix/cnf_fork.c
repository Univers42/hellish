/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   cnf_fork.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/30 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/30 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "execution_private.h"

/* Run command_not_found_handle the way bash runs it: in a SUBSHELL.
**
** That is worth stating because it is the opposite of what a shell function
** normally gets here, and it was measured rather than assumed --
**
**     command_not_found_handle() { X=set; cd /tmp; }
**     nosuchcmd; echo "X=$X"; pwd
**
** prints an empty X and the ORIGINAL directory under bash 5.3.9, in
** --posix mode and out of it.  A handler cannot change the shell it runs
** in; it can only print, and set an exit status.  Running it in the parent
** would have been easier and would have quietly given plugins a power bash
** never granted them -- the kind of divergence that shows up much later as
** "the shell cd'd somewhere on a typo".
**
** What must NOT move into the child is the dispatch decision itself: the
** "command not found" diagnostic is printed by the exec child (exe_error.c),
** so by the time that process exists it is already too late to replace the
** message.  Hence the split -- decide in the parent, run in the fork.
**
** The status is the handler's own (`return 42` yields 42), which falls out
** of exiting with what execute_func_call returned.  The child exits through
** exit(), not exit_clean(), for the same reason fork_compound does: it holds
** a copy of the parent's t_shell and must not free state the parent owns.
*/
t_execution_state	cnf_fork_hook(t_shell *state, t_executable_cmd *cmd,
						t_executable_node *exe)
{
	int	pid;

	pid = fork();
	if (pid == 0)
	{
		jc_child(state);
		default_signal_handlers();
		set_up_redirection(state, exe);
		exit(execute_func_call(state,
				func_lookup(state, CNF_HOOK), &cmd->argv).status);
	}
	jc_parent(state, pid);
	free_executable_cmd(state, *cmd);
	free_executable_node(exe);
	return (res_pid(pid));
}
