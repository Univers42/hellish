/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   prompt.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/07 15:44:40 by marvin            #+#    #+#             */
/*   Updated: 2026/01/07 15:44:40 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

# include "../incs/prompt.h"

void bg_readline(int outfd, char *prompt)
{
	char *ret;

	rl_instream = stdin;
	rl_outstream = stderr;
	ret = readline(prompt);
	if (!ret)
	{
		close(outfd);
		exit(1);
	}
	write_to_file(ret, outfd);
	close(outfd);
	exit(0);
}

int attach_input_readline(t_rl *l, int pp[2], int pid)
{
	int status;

	close(pp[1]);
	vec_push_fd(&l->str, pp[0]);
	buff_readline_update(l);
	close(pp[0]);
	while (1)
		if (waitpid(pid, &status, 0) != -1)
			break;
	if (WIFSIGNALED(status))
	{
		ft_eprintf("\n");
		return (2);
	}
	return (WEXITSTATUS(status));
}

int get_more_input_readline(t_rl *l, char *prompt)
{
	int pp[2];
	int pid;
	t_sigaction old_sigint, old_sigquit, ign;

	if (pipe(pp))
		critical_error_errno_context("pipe");
	pid = fork();
	if (pid == 0)
	{
		/* child restores desired handlers for readline */
		readline_bg_signals();
		close(pp[0]);
		bg_readline(pp[1], prompt);
	}
	else if (pid < 0)
		critical_error_errno_context("fork");
	else
	{
		int ret = attach_input_readline(l, pp, pid);
		/* restore parent's original signal handlers if we changed them */
		if (rc == NULL || rc->handle_signals)
		{
			sigaction(SIGINT, &old_sigint, NULL);
			sigaction(SIGQUIT, &old_sigquit, NULL);
		}
		return (ret);
	}
	ft_assert("Unreachable" == 0);
	return (0);
}

void buff_readline_update(t_rl *l)
{
	l->has_line = l->cursor != l->str.len;
}

void buff_readline_reset(t_rl *l)
{
	ft_memmove(l->str.buff, l->str.buff + l->cursor, l->str.len - l->cursor);
	l->str.len -= l->cursor;
	if (l->str.buff)
		l->str.buff[l->str.len] = 0;
	l->cursor = 0;
	buff_readline_update(l);
}

void buff_readline_init(t_rl *ret)
{
	*ret = (t_rl){0};
}

void update_context(t_rl *rl, char **context, char **base_context)
{
	char *new_ctx;
	int len;

	if (!rl->should_update_ctx)
		return;
	len = ft_strlen(*base_context) + 16;
	new_ctx = (char *)malloc((size_t)len);
	if (!new_ctx)
		return;
	snprintf(new_ctx, (size_t)len, "%s: line %d", *base_context, rl->lineno);
	if (*context)
		free(*context);
	*context = new_ctx;
}

int get_more_input_notty(t_rl *rl)
{
	char buff[4096 * 2];
	int ret;
	int status;

	status = 1;
	while (1)
	{
		ret = read(0, buff, sizeof(buff));
		if (ret < 0 && errno == EINTR)
			status = 2;
		if (ret == 0)
			rl->has_finished = true;
		if (ret == 0)
			vec_push(&rl->str, "\n");
		if (ret <= 0)
			break;
		status = 0;
		vec_push(&rl->str, buff, ret);
		if (ft_strnchr(buff, '\n', ret))
			break;
	}
	buff_readline_update(rl);
	return (status);
}