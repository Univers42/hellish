/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   capture_subshell_output.c                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 19:35:39 by marvin            #+#    #+#             */
/*   Updated: 2026/01/22 19:35:39 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"
#include "sys.h"

static pid_t	fork_and_exec_sh(t_shell *state, int pipefd[2], const char *cmd)
{
	pid_t		pid;
	char *const	argv[]
		= {(char *)PATH_HELLISH, (char *)"-c", (char *)cmd, NULL};
	char		**envp;

	pid = fork();
	if (pid == -1)
		return (-1);
	if (pid == 0)
	{
		close(pipefd[0]);
		dup2(pipefd[1], STDOUT_FILENO);
		close(pipefd[1]);
		envp = get_envp(state, PATH_HELLISH);
		execve(PATH_HELLISH, argv, envp);
		if (envp)
			free_tab(envp);
		exit(127);
	}
	return (pid);
}

static char	*read_pipe_and_wait(pid_t pid, int readfd)
{
	t_string	out;
	char		*ret;
	size_t		nlen;
	int			status;

	vec_init(&out);
	out.elem_size = 1;
	vec_append_fd(readfd, &out);
	close(readfd);
	while (waitpid(pid, &status, 0) == -1 && errno == EINTR)
		;
	ret = malloc(out.len + 1);
	if (!ret)
		return (free(out.ctx), ft_strdup(""));
	if (out.len)
		ft_memcpy(ret, out.ctx, out.len);
	ret[out.len] = '\0';
	nlen = out.len;
	while (nlen > 0 && ret[nlen - 1] == '\n')
		nlen--;
	ret[nlen] = '\0';
	free(out.ctx);
	return (ret);
}

char	*capture_subshell_output(t_shell *state, const char *cmd)
{
	int		pipefd[2];
	pid_t	pid;

	if (is_empty_command(cmd))
		return (ft_strdup(""));
	if (pipe(pipefd) == -1)
		return (ft_strdup(""));
	pid = fork_and_exec_sh(state, pipefd, cmd);
	if (pid == -1)
	{
		close(pipefd[0]);
		close(pipefd[1]);
		return (ft_strdup(""));
	}
	close(pipefd[1]);
	return (read_pipe_and_wait(pid, pipefd[0]));
}
