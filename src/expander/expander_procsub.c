/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   expander_procsub.c                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/16 12:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/16 12:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "shell.h"
#include "expander.h"
#include "env.h"
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <stdio.h>

static char	*create_procsub_input(t_shell *state, const char *cmd)
{
	int				pipefd[2];
	pid_t			pid;
	char			buf[64];
	t_procsub_entry	entry;

	if (!cmd || !*cmd)
		return (ft_strdup("/dev/null"));
	if (pipe(pipefd) == -1)
		return (NULL);
	pid = fork();
	if (pid == -1)
	{
		close(pipefd[0]);
		close(pipefd[1]);
		return (NULL);
	}
	if (pid == 0)
	{
		close(pipefd[0]);
		dup2(pipefd[1], STDOUT_FILENO);
		close(pipefd[1]);
		{
			/* Prefer current minishell to preserve builtin behavior */
			char *const argv_ms[] = {"/proc/self/exe", "-c", (char *)cmd, NULL};
			char *const argv_sh[] = {"/bin/sh", "-c", (char *)cmd, NULL};
			char **envp = get_envp(state, "/proc/self/exe");
			execve("/proc/self/exe", argv_ms, envp);
			execve("/bin/sh", argv_sh, envp);
			if (envp)
				free_tab(envp);
			_exit(127);
		}
	}
	close(pipefd[1]);
	snprintf(buf, sizeof(buf), "/dev/fd/%d", pipefd[0]);
	entry.pid = pid;
	entry.fd = pipefd[0];
	entry.path = ft_strdup(buf);
	vec_push(&state->proc_subs, &entry);
	return (ft_strdup(buf));
}

static char	*create_procsub_output(t_shell *state, const char *cmd)
{
	int				pipefd[2];
	pid_t			pid;
	char			buf[64];
	t_procsub_entry	entry;

	if (!cmd || !*cmd)
		return (ft_strdup("/dev/null"));
	if (pipe(pipefd) == -1)
		return (NULL);
	pid = fork();
	if (pid == -1)
	{
		close(pipefd[0]);
		close(pipefd[1]);
		return (NULL);
	}
	if (pid == 0)
	{
		close(pipefd[1]);
		dup2(pipefd[0], STDIN_FILENO);
		close(pipefd[0]);
		{
			/* Prefer current minishell to preserve builtin behavior */
			char *const argv_ms[] = {"/proc/self/exe", "-c", (char *)cmd, NULL};
			char *const argv_sh[] = {"/bin/sh", "-c", (char *)cmd, NULL};
			char **envp = get_envp(state, "/proc/self/exe");
			execve("/proc/self/exe", argv_ms, envp);
			execve("/bin/sh", argv_sh, envp);
			if (envp)
				free_tab(envp);
			_exit(127);
		}
	}
	close(pipefd[0]);
	snprintf(buf, sizeof(buf), "/dev/fd/%d", pipefd[1]);
	entry.pid = pid;
	entry.fd = pipefd[1];
	entry.path = ft_strdup(buf);
	vec_push(&state->proc_subs, &entry);
	return (ft_strdup(buf));
}

char	*expand_proc_sub(t_shell *state, t_ast_node *node)
{
	t_token		*tok;
	t_ast_node	*cmd_word;
	t_string	cmd_str;
	char		*result;
	bool		is_input;

	if (!node || node->node_type != AST_PROC_SUB || node->children.len < 2)
		return (NULL);
	tok = &((t_ast_node *)node->children.ctx)[0].token;
	cmd_word = &((t_ast_node *)node->children.ctx)[1];
	is_input = (tok->tt == TT_PROC_SUB_IN);
	cmd_str = word_to_string(*cmd_word);
	if (!cmd_str.ctx)
		return (ft_strdup("/dev/null"));
	if (!vec_ensure_space_n(&cmd_str, 1))
	{
		free(cmd_str.ctx);
		return (NULL);
	}
	((char *)cmd_str.ctx)[cmd_str.len] = '\0';
	if (is_input)
		result = create_procsub_input(state, (char *)cmd_str.ctx);
	else
		result = create_procsub_output(state, (char *)cmd_str.ctx);
	free(cmd_str.ctx);
	return (result);
}

void	cleanup_proc_subs(t_shell *state)
{
	size_t			i;
	t_procsub_entry	*entry;
	int				status;

	if (!state->proc_subs.ctx)
		return;
	i = 0;
	while (i < state->proc_subs.len)
	{
		entry = &((t_procsub_entry *)state->proc_subs.ctx)[i];
		if (entry->fd >= 0)
			close(entry->fd);
		if (entry->pid > 0)
			waitpid(entry->pid, &status, 0);
		free(entry->path);
		i++;
	}
	free(state->proc_subs.ctx);
	vec_init(&state->proc_subs);
	state->proc_subs.elem_size = sizeof(t_procsub_entry);
}
