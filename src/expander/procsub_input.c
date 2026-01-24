/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   procsub_input.c                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 19:18:11 by marvin            #+#    #+#             */
/*   Updated: 2026/01/22 19:18:11 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"
#include "sys.h"

/* fork child, wire child's stdout to pipefd[1],
exec preferred binary then /bin/sh */
static pid_t	fork_and_exec_procsub_input(t_shell *state, int pipefd[2],
						const char *cmd)
{
	pid_t		pid;
	char *const	argv_ms[] = {(char *)PROC_SELF_EXE, (char *)CMD_OPT,
		(char *)cmd, NULL};
	char *const	argv_sh[] = {(char *)PATH_HELLISH, (char *)CMD_OPT,
		(char *)cmd, NULL};
	char		**envp;

	pid = fork();
	if (pid == -1)
		return (-1);
	if (pid == 0)
	{
		close(pipefd[0]);
		dup2(pipefd[1], STDOUT_FILENO);
		close(pipefd[1]);
		envp = get_envp(state, PROC_SELF_EXE);
		execve(PROC_SELF_EXE, argv_ms, envp);
		execve(PATH_HELLISH, argv_sh, envp);
		if (envp)
			free_tab(envp);
		exit(127);
	}
	return (pid);
}

char	*create_procsub_input(t_shell *state, const char *cmd)
{
	int				pipefd[2];
	pid_t			pid;
	char			buf[64];
	t_procsub_entry	entry;

	if (!cmd || !*cmd)
		return (ft_strdup(BLACK_HOLE));
	if (pipe(pipefd) == -1)
		return (NULL);
	pid = fork_and_exec_procsub_input(state, pipefd, cmd);
	if (pid == -1)
	{
		close(pipefd[0]);
		close(pipefd[1]);
		return (NULL);
	}
	close(pipefd[1]);
	ft_snprintf(buf, sizeof(buf), "/dev/fd/%d", pipefd[0]);
	entry.pid = pid;
	entry.fd = pipefd[0];
	entry.path = ft_strdup(buf);
	vec_push(&state->proc_subs, &entry);
	return (ft_strdup(buf));
}
