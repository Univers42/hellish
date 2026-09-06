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
#include "sys.h"

/* Fork a child for a >(cmd) process substitution.  The child reads from
   pipefd[0] and runs the body in place; procsub_run_child (procsub_input.c)
   says why it is no longer a re-exec of the shell. */
static pid_t	fork_and_run_procsub(t_shell *state,
								int pipefd[2],
								const char *cmd)
{
	pid_t	pid;

	pid = fork();
	if (pid == -1)
		return (-1);
	if (pid == 0)
	{
		close(pipefd[1]);
		dup2(pipefd[0], STDIN_FILENO);
		close(pipefd[0]);
		procsub_run_child(state, cmd);
	}
	return (pid);
}

/* Create a >(cmd) process substitution: the shell writes to pipefd[1] and
   the child reads from pipefd[0] and runs `cmd`.  The parent gets back the
   /dev/fd/N path for pipefd[1] as a string it can use as a redirect target.
   The child entry is registered in state->proc_subs so cleanup_proc_subs
   can close the fd and reap the child once the outer command finishes. */
char	*create_procsub_output(t_shell *state, const char *cmd)
{
	int				pipefd[2];
	pid_t			pid;
	char			buf[64];
	t_procsub_entry	entry;

	if (!cmd || !*cmd)
		return (ft_strdup(BLACK_HOLE));
	if (pipe(pipefd) == -1)
		return (NULL);
	pid = fork_and_run_procsub(state, pipefd, cmd);
	if (pid == -1)
	{
		close(pipefd[0]);
		close(pipefd[1]);
		return (NULL);
	}
	close(pipefd[0]);
	ft_snprintf(buf, sizeof(buf), "/dev/fd/%d", pipefd[1]);
	entry.pid = pid;
	entry.fd = pipefd[1];
	entry.path = ft_strdup(buf);
	vec_push(&state->proc_subs, &entry);
	return (ft_strdup(buf));
}
