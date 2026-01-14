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
#include <wchar.h>

#include <readline/readline.h>
#include <sys/wait.h>
#include <unistd.h>
# include "helpers.h"

static int visible_width_cstr(const char *s);

/* exit codes:
*
* 0 - verything ok
*
* 1 - ctrl-d
*
* 2 - ctrl-c / ctrl-\
*/
void bg_readline(int outfd, char *prompt)
{
	char	*ret;

	rl_instream = stdin;
	rl_outstream = stderr;
	if (getenv("MINISHELL_DEBUG_PROMPT"))
	{
		fprintf(stderr, "[DEBUG PROMPT] bytes: ");
		for (size_t i = 0; prompt[i]; ++i)
			fprintf(stderr, "%02x ", (unsigned char)prompt[i]);
		fprintf(stderr, "\n");
		fprintf(stderr, "[DEBUG PROMPT] visible width: %d\n", visible_width_cstr(prompt));
	}
	ret = readline(prompt);
	if (!ret)
	{
		close(outfd);
		exit (1);
	}
	write_to_file(ret, outfd);
	free(ret);
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

static int visible_width_cstr(const char *s)
{
    mbstate_t st;
    size_t i = 0;
    int width = 0;
    memset(&st, 0, sizeof(st));
    while (s[i])
    {
        if (s[i] == '\001') { i++; while (s[i] && s[i] != '\002') i++; if (s[i]) i++; continue; }
        if ((unsigned char)s[i] == 0x1b && s[i+1] == '[') { i+=2; while (s[i] && !(s[i] >= 'A' && s[i] <= 'z')) i++; if (s[i]) i++; continue; }
        wchar_t wc;
        size_t consumed = mbrtowc(&wc, s + i, MB_CUR_MAX, &st);
        if (consumed == (size_t)-1 || consumed == (size_t)-2 || consumed == 0) { width += 1; i++; memset(&st,0,sizeof(st)); continue; }
        int w = ft_wcwidth(wc);
        if (w <= 0) w = 1;
        width += w;
        i += consumed;
    }
    return width;
}
