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
#include "sh_alias.h"

static void	apply_alias(t_shell *state, t_executable_cmd *cmd)
{
	char	*name;
	char	*val;
	char	**words;
	t_vec	new_argv;
	int		i;
	char	*dup;

	if (cmd->argv.len == 0 || !cmd->argv.ctx)
		return ;
	name = ((char **)cmd->argv.ctx)[0];
	if (!name)
		return ;
	val = alias_get(&state->aliases, name);
	if (!val)
		return ;
	words = ft_split(val, ' ');
	if (!words || !words[0])
	{
		free_tab(words);
		return ;
	}
	vec_init(&new_argv);
	new_argv.elem_size = sizeof(char *);
	i = 0;
	while (words[i])
	{
		dup = ft_strdup(words[i]);
		vec_push(&new_argv, &dup);
		i++;
	}
	i = 1;
	while (i < (int)cmd->argv.len)
	{
		dup = ft_strdup(((char **)cmd->argv.ctx)[i]);
		vec_push(&new_argv, &dup);
		i++;
	}
	i = 0;
	while (i < (int)cmd->argv.len)
	{
		free(((char **)cmd->argv.ctx)[i]);
		i++;
	}
	free(cmd->argv.ctx);
	cmd->argv = new_argv;
	free_tab(words);
}

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

static t_execution_state	handle_func_call(t_shell *state,
						t_executable_cmd *cmd,
						t_executable_node *exe)
{
	t_execution_state	res;

	procsub_close_fds_parent(state);
	res = execute_func_call(state,
			func_lookup(state, ((char **)(cmd->argv.ctx))[0]),
			&cmd->argv);
	free_executable_cmd(*cmd);
	free_executable_node(exe);
	return (res);
}

static t_execution_state	handle_empty_command(t_shell *state,
						t_executable_cmd *cmd,
						t_executable_node *exe)
{
	ft_eprintf("%s: command not found\n", state->ctx);
	procsub_close_fds_parent(state);
	free_executable_cmd(*cmd);
	free_executable_node(exe);
	return (res_status(COMMAND_NOT_FOUND));
}

static t_execution_state	handle_assign_only(t_shell *state,
								t_executable_cmd *cmd,
								t_executable_node *exe)
{
	if (exe->modify_parent_ctx)
		env_extend(&state->env, &cmd->pre_assigns, false);
	procsub_close_fds_parent(state);
	free_executable_cmd(*cmd);
	free_executable_node(exe);
	return (res_status(state->last_cmdsub_status));
}

t_execution_state	execute_simple_command(t_shell *state,
									t_executable_node *exe)
{
	t_executable_cmd	cmd;

	state->last_cmdsub_status = 0;
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
	apply_alias(state, &cmd);
	if (cmd.argv.len > 0 && ((char **)cmd.argv.ctx)[0]
		&& ((char **)cmd.argv.ctx)[0][0] == '\0')
		return (handle_empty_command(state, &cmd, exe));
	if (cmd.argv.len && func_lookup(state, ((char **)(cmd.argv.ctx))[0])
		&& exe->modify_parent_ctx)
		return (handle_func_call(state, &cmd, exe));
	if (cmd.argv.len && builtin_func(((char **)(cmd.argv.ctx))[0])
		&& exe->modify_parent_ctx)
		return (execute_builtin_cmd_fg(state, &cmd, exe));
	else if (cmd.argv.len)
		return (execute_cmd_bg(state, exe, &cmd));
	else
		return (handle_assign_only(state, &cmd, exe));
}
