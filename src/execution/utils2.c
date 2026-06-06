/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   utils2.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 16:50:27 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/27 16:16:39 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "execution_private.h"
#include "sys.h"

/* Update the `_` (ULTIMATE_ARG) environment variable to the last word of
   the command.  POSIX says $_ is the last argument of the previous command;
   we set it before forking so both parent and child see the right value
   immediately after the command. */
void	set_last_underscore_var(t_shell *state, t_executable_cmd *cmd)
{
	char	*last;

	if (cmd->argv.len > 0)
	{
		last = ((char **)cmd->argv.ctx)[cmd->argv.len - 1];
		if (last)
			env_set(&state->env,
				env_create(ft_strdup(ULTIMATE_ARG), ft_strdup(last), true));
	}
}

/* Thin wrapper kept so callers have a single named function to call;
   makes it easy to add pre/post logic around `_` updates in one place. */
void	update_underscore_var(t_shell *state, t_executable_cmd *cmd)
{
	set_last_underscore_var(state, cmd);
}

/* True iff set_up_redirection would do any fd work for this node. Lets callers
   skip the dup/dup2/close save+restore dance entirely for a plain command with
   no redirections and no pipe fds (the common hot-loop case). */
int	fd_setup_needed(t_executable_node *exe)
{
	return (exe->next_infd != -1 || exe->outfd != 1
		|| exe->infd != 0 || exe->redirs.len != 0);
}

/* Save fds + apply redirections only when the command has any (no redir + no
   pipe fds → the hot case skips dup/dup2/close). Always clears the node's fd
   fields so teardown won't re-close them. Returns 1 if fds were saved. */
int	prep_redir(t_shell *state, t_executable_node *exe, int *bak, int persist)
{
	int	need;

	need = persist || fd_setup_needed(exe);
	if (need)
	{
		take_backup_fds(bak, persist);
		set_up_redirection(state, exe);
	}
	exe->infd = -1;
	exe->outfd = -1;
	exe->next_infd = -1;
	return (need);
}

/* Wire up all I/O for one command.  Order matters: pipe fds go first
   (next_infd is closed -- the parent already has it; outfd/infd are
   dup2'd to stdout/stdin), then explicit redirections from the redirs
   vec or the AST.  If we have a non-empty redirs vec we use the
   pre-resolved path (apply_redirs_from_vec); if the vec is empty but
   the AST node has children, we resolve lazily (apply_redirs_from_ast).
   Both paths ultimately call apply_redir_now. */
void	set_up_redirection(t_shell *state, t_executable_node *exe)
{
	if (exe->next_infd != -1)
		close(exe->next_infd);
	if (exe->outfd != 1)
		(dup2(exe->outfd, 1), close(exe->outfd));
	if (exe->infd != 0)
		(dup2(exe->infd, 0), close(exe->infd));
	if (exe->redirs.len == 0)
		return ;
	if (exe->redirs.ctx)
	{
		apply_redirs_from_vec(state, exe);
		return ;
	}
	if (exe->node)
	{
		apply_redirs_from_ast(state, exe);
		return ;
	}
	if (state->ctx)
		ft_eprintf(MSG_REDIR_DATA_ERR, state->ctx);
	else
		ft_eprintf(MSG_REDIR_DATA_ERR, "shell");
	exit(EXIT_GENERAL_ERR);
}
