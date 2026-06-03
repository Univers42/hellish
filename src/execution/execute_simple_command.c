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
#include "ft_builtins.h"

void	apply_alias(t_shell *state, t_executable_cmd *cmd);
void	replace_null_argv_with_empty(t_executable_cmd *cmd);
void	restore_fds(int *bak);

static t_execution_state	handle_func_call(t_shell *state,
						t_executable_cmd *cmd,
						t_executable_node *exe)
{
	t_execution_state	res;
	int					bak[3];
	t_vec				saves;

	bak[0] = dup(0);
	bak[1] = dup(1);
	bak[2] = dup(2);
	set_up_redirection(state, exe);
	exe->infd = -1;
	exe->outfd = -1;
	exe->next_infd = -1;
	procsub_close_fds_parent(state);
	saves = apply_temp_assigns(state, &cmd->pre_assigns);
	res = execute_func_call(state,
			func_lookup(state, ((char **)(cmd->argv.ctx))[0]),
			&cmd->argv);
	restore_temp_assigns(state, &saves);
	restore_fds(bak);
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
		env_extend(&state->env, &cmd->pre_assigns, state->opt_allexport);
	procsub_close_fds_parent(state);
	free_executable_cmd(*cmd);
	free_executable_node(exe);
	return (res_status(state->last_cmdsub_status));
}

static t_execution_state	dispatch_cmd(t_shell *state,
								t_executable_cmd *cmd,
								t_executable_node *exe)
{
	char	*argv0;

	if (cmd->argv.len == 0 || !cmd->argv.ctx)
		return (handle_assign_only(state, cmd, exe));
	argv0 = ((char **)cmd->argv.ctx)[0];
	if (argv0 && argv0[0] == '\0')
		return (handle_empty_command(state, cmd, exe));
	if (func_lookup(state, argv0) && exe->modify_parent_ctx)
		return (handle_func_call(state, cmd, exe));
	if (builtin_func(argv0) && exe->modify_parent_ctx)
		return (execute_builtin_cmd_fg(state, cmd, exe));
	return (execute_cmd_bg(state, exe, cmd));
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
	if (state->opt_xtrace && cmd.argv.len > 0)
		xtrace_print(state, &cmd.argv);
	return (dispatch_cmd(state, &cmd, exe));
}
