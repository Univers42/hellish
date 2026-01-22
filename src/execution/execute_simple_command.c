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




t_exe_res execute_simple_command(t_shell *state, t_executable_node *exe)
{
	t_executable_cmd cmd;
	size_t i;
	char *first_arg;

	if (expand_simple_command(state, exe->node, &cmd, &exe->redirs))
	{
#if TRACE_DEBUG
		ft_eprintf("%s: debug: expand_simple_command failed\n", state->context);
#endif
		free_executable_cmd(cmd);
		free_executable_node(exe);
		if (get_g_sig()->should_unwind)
			return (res_status(CANCELED));
		return (res_status(AMBIGUOUS_REDIRECT));
	}
	if (!cmd.argv.ctx)
		cmd.argv.len = 0;
	// validate argv pointers to avoid crashes from corrupted/NULL entries
	i = 0;
	while (i < cmd.argv.len)
	{
		char *p = ((char **)cmd.argv.ctx)[i];
		if (p == NULL || (uintptr_t)p < 4096) /* small pointer -> invalid */
		{
			char *empty = ft_strdup("");
			((char **)cmd.argv.ctx)[i] = empty;
		}
		i++;
	}
	/* Check if first argument is empty string - treat as command not found */
	if (cmd.argv.len > 0)
	{
		first_arg = ((char **)cmd.argv.ctx)[0];
		if (first_arg && first_arg[0] == '\0')
		{
			/* Empty command name (e.g., "" or '') - command not found */
			ft_eprintf("%s: : command not found\n", state->context);
			free_executable_cmd(cmd);
			free_executable_node(exe);
			return (res_status(COMMAND_NOT_FOUND));
		}
	}
	if (cmd.argv.len && builtin_func(((char **)(cmd.argv.ctx))[0]) && exe->modify_parent_context)
		return (execute_builtin_cmd_fg(state, &cmd, exe));
	else if (cmd.argv.len)
		return (execute_cmd_bg(state, exe, &cmd));
	else
	{
		if (exe->modify_parent_context)
			env_extend(&state->env, &cmd.pre_assigns, false);
		free_executable_cmd(cmd);
		free_executable_node(exe);
		return (res_status(0));
	}
}
