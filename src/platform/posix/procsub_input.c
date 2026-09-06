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
#include "executor.h"
#include "ft_builtins.h"
#include "sys.h"

/* The child half of <(cmd) and >(cmd): run `cmd` IN THIS PROCESS, the
   fork's copy of the shell being the subshell.  Never returns.

   Both halves used to re-exec the shell binary with `-c cmd`, and a fresh
   process knows nothing the parent defined: no functions, no arrays, no
   `local`s, its own $$.  `while read -r x; do ...; done < <(some_function)`
   and `mapfile -t a < <(some_function)` said "command not found" and
   iterated zero times -- issue #119.  The command-substitution child
   (capture_subshell_output.c) already runs in process for exactly this
   reason; this is the same body.  Traps reset the way a subshell's do,
   so the parent's EXIT trap cannot fire from here.  cmdsub_in_place lets a
   body that is one external command execve without a second fork, so
   `cat <(seq 3)` costs one clone plus one exec, where the re-exec cost a
   whole shell start-up on top. */
void	procsub_run_child(t_shell *state, const char *cmd)
{
	reset_traps_child(state);
	state->cmdsub_in_place = cs_single_cmd(state, cmd);
	exit(exec_string(state, (char *)cmd) & 0xFF);
}

/* Fork a child for a <(cmd) process substitution.  The child connects its
   stdout to pipefd[1] and runs the body in place. */
static pid_t	fork_and_run_procsub_input(t_shell *state, int pipefd[2],
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
		procsub_run_child(state, cmd);
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
	pid = fork_and_run_procsub_input(state, pipefd, cmd);
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
