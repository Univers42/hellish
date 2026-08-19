/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   execute_builtin.c                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 15:11:14 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/27 16:38:09 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "execution_private.h"
#include "ft_builtins.h"

void	exit_clean(t_shell *state, int code);

/* `exec` with no command word just applies its redirections; POSIX keeps them
   in effect for the rest of the shell, so those fds must NOT be restored. */
static int	is_bare_exec(t_executable_cmd *cmd)
{
	return (cmd->argv.len == 1
		&& ft_strcmp(((char **)cmd->argv.ctx)[0], "exec") == 0);
}

/* A bare `exec` keeps its redirections, so we must NOT save/restore 0/1/2 for
   it: the saved dups can land on the very fd a redirection targets (e.g. 6) and
   closing them would tear the redirection back down. */
void	take_backup_fds(int *bak, int persist)
{
	if (persist)
		return ;
	bak[0] = save_fd(0);
	bak[1] = save_fd(1);
	bak[2] = save_fd(2);
}

void	restore_backup_fds(int *bak, int persist)
{
	if (persist)
		return ;
	dup2(bak[0], 0);
	dup2(bak[1], 1);
	dup2(bak[2], 2);
	close(bak[0]);
	close(bak[1]);
	close(bak[2]);
}

/* Release the command, then honour POSIX's rule that a failing SPECIAL
   builtin aborts a non-interactive shell.  The verdict has to be taken
   while argv is still alive, so it happens before the frees and the exit
   after them -- exit_clean tears down the session and must not run with
   this command's memory still outstanding. */
static t_execution_state	finish_builtin(t_shell *state,
		t_executable_cmd *cmd, t_executable_node *exe, int status)
{
	bool	fatal;

	fatal = strict_builtin_failed(state, cmd, status);
	procsub_close_fds_parent(state);
	free_executable_cmd(state, *cmd);
	free_executable_node(exe);
	if (fatal)
		exit_clean(state, status);
	return (res_status(status));
}

/* Run a builtin IN THE PARENT process (the only way its side effects --
   cd, export, read, set -- reach the caller).  The redirection dance is:
   save 0/1/2, redirect, run, restore.  The persist flag skips both save
   and restore for a bare `exec`: that command's whole point is to keep
   the redirections alive, so restoring them would undo the effect.
   Pre-assignments (NAME=val before the command) are applied temporarily
   around the call and rolled back by restore_temp_assigns so they don't
   pollute the environment after the builtin returns. */
t_execution_state	execute_builtin_cmd_fg(t_shell *state,
								t_executable_cmd *cmd,
								t_executable_node *exe)
{
	int		bak[3];
	int		status;
	int		persist;
	int		need;
	t_vec	saves;

	persist = is_bare_exec(cmd);
	need = prep_redir(state, exe, bak, persist);
	update_underscore_var(state, cmd);
	saves = apply_temp_assigns(state, &cmd->pre_assigns);
	status = builtin_func(((char **)(cmd->argv.ctx))[0])(state, cmd->argv);
	restore_temp_assigns(state, &saves);
	if (need)
		restore_backup_fds(bak, persist);
	return (finish_builtin(state, cmd, exe, status));
}
