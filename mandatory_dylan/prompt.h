/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   prompt.h                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/06 22:15:26 by marvin            #+#    #+#             */
/*   Updated: 2026/01/06 22:15:26 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef PROMPT_H
# define PROMPT_H

#include "common.h"
#include "readline/readline.h"

# define ANSI_RED "\001\033[31m\002"
# define ANSI_GREEN "\001\033[32m\002"
# define ANSI_RESET "\001\033[0m\002"

# define RL_SPACER_1 "\x03"

# define SQUOTE_PROMPT "squote>"
# define DQUOTE_PROMPT "dquote> "
# define BQUOTE_PROMPT "bquote> "

typedef struct s_rl
{
	bool	has_line;
	bool	should_update_context;
	bool	has_finished;
	int		line;
	t_string	buff;
	size_t	cursor;
}	t_rl;


static inline void bg_readline(int outfd, char *prompt)
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

static inline int attach_input_readline(t_rl *l, int pp[2], int pid)
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

static inline int get_more_input_readline(t_rl *l, char *prompt)
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

static inline void buff_readline_update(t_rl *l)
{
	l->has_line = l->cursor != l->str.len;
}

static inline void buff_readline_reset(t_rl *l)
{
	ft_memmove(l->str.buff, l->str.buff + l->cursor, l->str.len - l->cursor);
	l->str.len -= l->cursor;
	if (l->str.buff)
		l->str.buff[l->str.len] = 0;
	l->cursor = 0;
	buff_readline_update(l);
}

static inline void buff_readline_init(t_rl *ret)
{
	*ret = (t_rl){0};
}

static inline void update_context(t_rl *rl, char **context, char **base_context)
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

static inline int get_more_input_notty(t_rl *rl)
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
#endif