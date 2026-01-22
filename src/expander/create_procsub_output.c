/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   create_procsub_output.c                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 19:08:40 by marvin            #+#    #+#             */
/*   Updated: 2026/01/22 19:08:40 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"

static pid_t	fork_and_exec_procsub(t_shell *state,
								int pipefd[2],
								const char *cmd)
{
	pid_t		pid;
	char *const	argv_ms[]
		= {(char *)"/proc/self/exe", (char *)"-c", (char *)cmd, NULL};
	char *const	argv_sh[]
		= {(char *)"/bin/sh", (char *)"-c", (char *)cmd, NULL};
	char		**envp;

	pid = fork();
	if (pid == -1)
		return (-1);
	if (pid == 0)
	{
		close(pipefd[1]);
		dup2(pipefd[0], STDIN_FILENO);
		close(pipefd[0]);
		envp = get_envp(state, "/proc/self/exe");
		execve("/proc/self/exe", argv_ms, envp);
		execve("/bin/sh", argv_sh, envp);
		if (envp)
			free_tab(envp);
		_exit(127);
	}
	return (pid);
}

char	*create_procsub_output(t_shell *state, const char *cmd)
{
	int				pipefd[2];
	pid_t			pid;
	char			buf[64];
	t_procsub_entry	entry;

	if (!cmd || !*cmd)
		return (ft_strdup("/dev/null"));
	if (pipe(pipefd) == -1)
		return (NULL);
	pid = fork_and_exec_procsub(state, pipefd, cmd);
	if (pid == -1)
	{
		close(pipefd[0]);
		close(pipefd[1]);
		return (NULL);
	}
	close(pipefd[0]);
	snprintf(buf, sizeof(buf), "/dev/fd/%d", pipefd[1]);
	entry.pid = pid;
	entry.fd = pipefd[1];
	entry.path = ft_strdup(buf);
	vec_push(&state->proc_subs, &entry);
	return (ft_strdup(buf));
}
