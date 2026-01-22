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

char	*create_procsub_input(t_shell *state, const char *cmd)
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
