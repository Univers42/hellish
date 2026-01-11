/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   buffered_readline_readline.c                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 23:32:56 by marvin            #+#    #+#             */
/*   Updated: 2026/01/09 23:32:56 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "shell.h"
#include <stdio.h>

#include <readline/readline.h>
#include <sys/wait.h>
#include <unistd.h>

/* exit codes:
*
* 0 - verything ok
*
* 1 - ctrl-d
*
* 2 - ctrl-c / ctrl-\
*/
void	bg_readline(int outfd, char *prompt)
{
	char	*ret;

	rl_instream = stdin;
	rl_outstream = stderr;
	ret = readline(prompt);
	if (!ret)
	{
		close(outfd);
		exit (1);
	}
	write_to_file(ret, outfd);
	close(outfd);
	exit(0);
}

int	attach_input_readline(t_buff_readline *l, int pp[2], int pid)
{
	int	status;

	close(pp[1]);
	vec_append_fd(pp[0], &l->buff);
	buff_readline_update(l);
	close(pp[0]);
	while (1)
		if (waitpid(pid, &status, 0) != -1)
			break ;
	if (WIFSIGNALED(status))
	{
		ft_eprintf("\n");
		return (2);
	}
	return (WEXITSTATUS(status));
}

int	get_more_input_readline(t_buff_readline *l, char *prompt)
{
	int	pp[2];
	int	pid;

	if (pipe(pp))
		critical_error_errno_context("pipe");
	pid = fork();
	if (pid == 0)
	{
		readline_bg_signals();
		close(pp[0]);
		bg_readline(pp[1], prompt);
	}
	else if (pid < 0)
		critical_error_errno_context("fork");
	else
	{
		return (attach_input_readline(l, pp, pid));
	}
	ft_assert("Unreachable" == 0);
	return (0);
}
