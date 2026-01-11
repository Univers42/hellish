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

#include "shell.h"
#include <stdbool.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "libft.h"
#include <stdint.h>
# include "env.h"
# include "expander.h"

// returns status
int	actually_run(t_shell *state, t_vec *args)
{
	char	*path_of_exe;
	char	**envp;
	int		status;

	ft_assert(args->len >= 1);
#if TRACE_DEBUG
	// debug
	ft_eprintf("%s: debug: actually_run argv[0]=%s argc=%lu\n",
		state->context, ((char **)args->ctx)[0], (unsigned long)args->len);
#endif
	if (builtin_func(((char **)(args->ctx))[0]))
		exit(builtin_func(((char **)(args->ctx))[0])(state, *args));
	status = find_cmd_path(state, ((char **)(args->ctx))[0], &path_of_exe);
	if (status == COMMAND_NOT_FOUND)
		return 127;
	if (status == EXE_PERM_DENIED)
		return 126;
	if (status)
		return status;
#if TRACE_DEBUG
	ft_eprintf("%s: debug: exec path=%s\n", state->context, path_of_exe);
#endif
	char *null_ptr = NULL;
	vec_push(args, &null_ptr);
	envp = get_envp(state, path_of_exe);
	execve(path_of_exe, (char **)(args->ctx), envp);
	/* If execve failed with ENOEXEC (Exec format error), try /bin/sh file args... */
	if (errno == ENOEXEC)
	{
		size_t	orig_elems;
		size_t	new_elems;
		char	**newargv;
		size_t	i;

		orig_elems = args->len - 1; /* exclude final NULL sentinel pushed above */
		new_elems = orig_elems + 1; /* add "/bin/sh" at front */
		newargv = malloc(sizeof(char *) * (new_elems + 1));
		if (newargv)
		{
			newargv[0] = "/bin/sh";
			newargv[1] = path_of_exe;
			i = 1;
			while (i < orig_elems)
			{
				newargv[i + 1] = ((char **)(args->ctx))[i];
				i++;
			}
			newargv[new_elems] = NULL;
			execve("/bin/sh", newargv, envp);
			free(newargv);
		}
		/* if we fall through, the /bin/sh exec failed as well */
	}
	/* Preserve errno, optionally print permission error, then free allocations */
	{
		int _saved_errno = errno;
		if (_saved_errno == EACCES)
			ft_eprintf("%s: %s: %s\n", state->context, path_of_exe, strerror(_saved_errno));
		/* free argv strings and buffer (child only) */
		if (args->ctx) {
			size_t _i = 0;
			char **_arr = (char **)args->ctx;
			while (_i < args->len) {
				if (_arr[_i]) free(_arr[_i]);
				_i++;
			}
			free(args->ctx);
		}
		free(path_of_exe);
		free_tab(envp);
		errno = _saved_errno;
	}
	// Match bash exit codes for execve errors
	if (errno == EACCES)
		return 126;
	if (errno == ENOENT)
		return 127;
	if (errno == ENOEXEC)
		return 126;
	if (errno == ENOTDIR)
		return 127;
	if (errno == ENOMEM)
		return 127;
	if (errno == EISDIR)
		return 126;
	if (errno == ENOENT || errno == ENOTDIR)
		return 127;
	if (errno == EACCES)
		return 126;
	if (errno == ENOEXEC)
		return 126;
	if (errno == ENOMEM)
		return 127;
	if (errno == EISDIR)
		return 126;
	return 1;
}

void	set_up_redirection(t_shell *state, t_executable_node *exe)
{
	t_redir	redir;
	size_t	i;

	if (exe->next_infd != -1)
		close(exe->next_infd);
	dup2(exe->outfd, 1);
	dup2(exe->infd, 0);

	/* nothing to do */
	if (exe->redirs.len == 0)
		return ;

	/* Case A: redir indices available in exe->redirs.ctx */
	if (exe->redirs.ctx)
	{
		i = 0;
		while (i < exe->redirs.len)
		{
			int idx = *(int *)vec_idx(&exe->redirs, i++);
			if (idx < 0 || !state->redirects.ctx || (size_t)idx >= state->redirects.len)
			{
				ft_eprintf("%s: internal error: invalid redirect index %d\n",
					state->context ? state->context : "minishell", idx);
				_exit(1);
			}
			redir = ((t_redir *)state->redirects.ctx)[(size_t)idx];
			if (redir.direction_in)
				dup2(redir.fd, 0);
			else
				dup2(redir.fd, 1);
			close(redir.fd);
		}
		return ;
	}

	/* Case B: no redirs buffer available — fallback: scan AST children for redirects */
	if (exe->node)
	{
		/* children[0] is command/subshell, redirects start at index 1 */
		for (i = 1; i < exe->node->children.len; ++i)
		{
			t_ast_node *curr = (t_ast_node *)vec_idx(&exe->node->children, i);
			if (curr->node_type != AST_REDIRECT)
				continue;
			int idx;
			if (redirect_from_ast_redir(state, curr, &idx))
			{
				/* ambiguous redirect or error: abort child */
				ft_eprintf("%s: ambiguous redirect\n", state->context);
				_exit(1);
			}
			if (idx < 0 || !state->redirects.ctx || (size_t)idx >= state->redirects.len)
			{
				ft_eprintf("%s: internal error: invalid redirect index %d\n",
					state->context ? state->context : "minishell", idx);
				_exit(1);
			}
			redir = ((t_redir *)state->redirects.ctx)[(size_t)idx];
			if (redir.direction_in)
				dup2(redir.fd, 0);
			else
				dup2(redir.fd, 1);
			close(redir.fd);
		}
		return ;
	}

	/* If we reach here, redirection info is inconsistent */
	ft_eprintf("%s: internal error: redirects present but no redirect data\n",
		state->context ? state->context : "minishell");
	_exit(1);
}

t_exe_res	execute_builtin_cmd_fg(t_shell *state, t_executable_cmd *cmd,
	t_executable_node *exe)
{
	int		stdin_bak;
	int		stdout_bak;
	int		status;

	stdin_bak = dup(0);
	stdout_bak = dup(1);
	set_up_redirection(state, exe);
	status = builtin_func(((char **)(cmd->argv.ctx))[0])(state, cmd->argv);
	dup2(stdin_bak, 0);
	dup2(stdout_bak, 1);
	close(stdin_bak);
	close(stdout_bak);
	free_executable_cmd(*cmd);
	free_executable_node(exe);
	return (res_status(status));
}

t_exe_res	execute_cmd_bg(t_shell *state,
		t_executable_node *exe, t_executable_cmd *cmd)
{
	int	pid;

	pid = fork();
	if (pid == 0)
	{
		default_signal_handlers();
		set_up_redirection(state, exe);
		env_extend(&state->env, &cmd->pre_assigns, true);
		exit(actually_run(state, &cmd->argv));
	}
	free_executable_cmd(*cmd);
	free_executable_node(exe);
	return (res_pid(pid));
}

t_exe_res	execute_simple_command(t_shell *state, t_executable_node *exe)
{
	t_executable_cmd	cmd;
	size_t				i;

	if (expand_simple_command(state, exe->node, &cmd, &exe->redirs))
	{
#if TRACE_DEBUG
		ft_eprintf("%s: debug: expand_simple_command failed\n", state->context);
#endif
		free_executable_cmd(cmd);
		free_executable_node(exe);
		if (g_should_unwind)
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
	if (cmd.argv.len && builtin_func(((char **)(cmd.argv.ctx))[0])
		&& exe->modify_parent_context)
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
