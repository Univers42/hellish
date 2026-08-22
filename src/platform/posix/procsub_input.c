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

/* Re-exec THIS shell to run `cmd`, in an already-forked child.  Never
   returns: it execs, or exits 127 the way any failed exec would.

   Both halves of process substitution used to exec the literal
   "/proc/self/exe" -- twice, under two names that expanded to the same
   string, so the second attempt was dead code -- and that path exists on
   Linux and nowhere else.  On macOS every <(cmd) and >(cmd) exec'd
   something that was not there and produced nothing.  self_exe_path()
   asks the kernel, and has an answer on Darwin too.

   A NULL from it means the kernel will not say where we are.  There is
   nothing sensible to exec at that point, and inventing a path would run
   the WRONG shell rather than fail, so the child just exits. */
void	procsub_exec_self(t_shell *state, const char *cmd)
{
	char	*self;
	char	*argv[4];
	char	**envp;

	self = self_exe_path();
	if (self)
	{
		argv[0] = self;
		argv[1] = (char *)CMD_OPT;
		argv[2] = (char *)cmd;
		argv[3] = NULL;
		envp = get_envp_all(state, self);
		execve(self, argv, envp);
		if (envp)
			free_tab(envp);
	}
	exit(127);
}

/* Fork a child for a <(cmd) process substitution.  The child connects its
   stdout to pipefd[1] and re-execs the shell. */
static pid_t	fork_and_exec_procsub_input(t_shell *state, int pipefd[2],
						const char *cmd)
{
	pid_t	pid;

	pid = fork();
	if (pid == -1)
		return (-1);
	if (pid == 0)
	{
		close(pipefd[0]);
		dup2(pipefd[1], STDOUT_FILENO);
		close(pipefd[1]);
		procsub_exec_self(state, cmd);
	}
	return (pid);
}

/* Create a <(cmd) process substitution: the child writes to pipefd[1],
   the parent keeps pipefd[0] open and returns its /dev/fd/N path.  The
   caller (expand_proc_sub) uses that path as a word that expands to a
   readable fd the outer command can open for its input. */
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
