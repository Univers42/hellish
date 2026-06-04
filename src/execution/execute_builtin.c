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
static void	take_backup_fds(int *bak, int persist)
{
	if (persist)
		return ;
	bak[0] = dup(0);
	bak[1] = dup(1);
	bak[2] = dup(2);
}

static void	restore_backup_fds(int *bak, int persist)
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

t_execution_state	execute_builtin_cmd_fg(t_shell *state,
								t_executable_cmd *cmd,
								t_executable_node *exe)
{
	int		bak[3];
	int		status;
	int		persist;
	t_vec	saves;

	persist = is_bare_exec(cmd);
	take_backup_fds(bak, persist);
	set_up_redirection(state, exe);
	exe->infd = -1;
	exe->outfd = -1;
	exe->next_infd = -1;
	update_underscore_var(state, cmd);
	saves = apply_temp_assigns(state, &cmd->pre_assigns);
	status = builtin_func(((char **)(cmd->argv.ctx))[0])(state, cmd->argv);
	restore_temp_assigns(state, &saves);
	restore_backup_fds(bak, persist);
	procsub_close_fds_parent(state);
	free_executable_cmd(*cmd);
	free_executable_node(exe);
	return (res_status(status));
}
