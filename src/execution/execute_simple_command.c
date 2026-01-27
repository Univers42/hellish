/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   execute_simple_command.c                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 23:30:52 by marvin            #+#    #+#             */
/*   Updated: 2026/01/09 23:30:52 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "execution_private.h"

static void	replace_null_argv_with_empty(t_executable_cmd *cmd)
{
	size_t	i;
	char	*p;

	i = 0;
	while (i < cmd->argv.len)
	{
		p = ((char **)cmd->argv.ctx)[i];
		if (p == NULL || (uintptr_t)p < 4096)
			((char **)cmd->argv.ctx)[i] = ft_strdup("");
		i++;
	}
}

static bool	is_empty_command_name(t_executable_cmd *cmd)
{
	char	*first_arg;

	if (cmd->argv.len == 0)
		return (false);
	first_arg = ((char **)cmd->argv.ctx)[0];
	return (first_arg && first_arg[0] == '\0');
}

static t_execution_state	handle_empty_command(t_shell *state,
						t_executable_cmd *cmd,
						t_executable_node *exe)
{
	ft_eprintf("%s: command not found\n", state->context);
	procsub_close_fds_parent(state);
	free_executable_cmd(*cmd);
	free_executable_node(exe);
	return (res_status(COMMAND_NOT_FOUND));
}

static t_execution_state	handle_assign_only(t_shell *state,
								t_executable_cmd *cmd,
								t_executable_node *exe)
{
	if (exe->modify_parent_context)
		env_extend(&state->env, &cmd->pre_assigns, false);
	procsub_close_fds_parent(state);
	free_executable_cmd(*cmd);
	free_executable_node(exe);
	return (res_status(0));
}

t_execution_state	execute_simple_command(t_shell *state, t_executable_node *exe)
{
	t_executable_cmd	cmd;

	if (expand_simple_command(state, exe->node, &cmd, &exe->redirs))
	{
		procsub_close_fds_parent(state);
		free_executable_cmd(cmd);
		free_executable_node(exe);
		if (get_g_sig()->should_unwind)
			return (res_status(CANCELED));
		return (res_status(AMBIGUOUS_REDIRECT));
	}
	if (!cmd.argv.ctx)
		cmd.argv.len = 0;
	replace_null_argv_with_empty(&cmd);
	if (is_empty_command_name(&cmd))
		return (handle_empty_command(state, &cmd, exe));
	if (cmd.argv.len && builtin_func(((char **)(cmd.argv.ctx))[0])
		&& exe->modify_parent_context)
		return (execute_builtin_cmd_fg(state, &cmd, exe));
	else if (cmd.argv.len)
		return (execute_cmd_bg(state, exe, &cmd));
	else
		return (handle_assign_only(state, &cmd, exe));
}
