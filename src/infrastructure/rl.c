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

#include "rl_private.h"

void	bg_readline(int outfd, char *prompt)
{
	char	*ret;
	size_t	i;

	rl_instream = stdin;
	rl_outstream = stderr;
	if (getenv("MINISHELL_DEBUG_PROMPT"))
	{
		fprintf(stderr, "[DEBUG PROMPT] bytes: ");
		i = -1;
		while (prompt[++i])
			fprintf(stderr, "%02x ", (unsigned char)prompt[i]);
		fprintf(stderr, "\n");
		fprintf(stderr, "[DEBUG PROMPT] visible width: %d\n",
			visible_width_cstr(prompt));
	}
	ret = readline(prompt);
	if (!ret)
		(close(outfd), exit (1));
	(write_to_file(ret, outfd), free(ret), close(outfd), exit(0));
}

int	attach_input_readline(t_rl *l, int pp[2], int pid)
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

int	get_more_input_readline(t_rl *l, char *prompt)
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
		return (attach_input_readline(l, pp, pid));
	ft_assert(0);
	return (0);
}
