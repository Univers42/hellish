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
#include "prompt_styles.h"
#include <locale.h>

void	setup_completion(void);
void	setup_vi_mode(void);
void	setup_emacs_mode(void);
int		select_prompt_style(void);

/* Print every line of `prompt` except the last to rl_outstream, stripping the
   \001/\002 width markers (they must not reach the terminal). Returns the final
   line. readline only ever sees a single-line prompt, which avoids the cursor
   drift and heavy full-line redraws that a multi-line prompt (embedded \n)
   causes during ↑/↓ history navigation. */
static char	*split_prompt(char *prompt)
{
	char	*nl;
	size_t	i;

	nl = ft_strrchr(prompt, '\n');
	if (!nl)
		return (prompt);
	i = 0;
	while (prompt + i <= nl)
	{
		if (prompt[i] != '\001' && prompt[i] != '\002')
			fputc((unsigned char)prompt[i], rl_outstream);
		i++;
	}
	fflush(rl_outstream);
	return (nl + 1);
}

static void	debug_dump_prompt(char *prompt)
{
	size_t	i;

	if (!getenv("MINISHELL_DEBUG_PROMPT"))
		return ;
	fprintf(stderr, "[DEBUG PROMPT] bytes: ");
	i = -1;
	while (prompt[++i])
		fprintf(stderr, "%02x ", (unsigned char)prompt[i]);
	fprintf(stderr, "\n");
	fprintf(stderr, "[DEBUG PROMPT] visible width: %d\n",
		visible_width_cstr(prompt));
}

void	bg_readline(int outfd, char *prompt, int edit_mode)
{
	char	*ret;

	setlocale(LC_ALL, "");
	rl_instream = stdin;
	rl_outstream = stderr;
	setup_completion();
	if (edit_mode == 0)
		setup_vi_mode();
	else
		setup_emacs_mode();
	debug_dump_prompt(prompt);
	prompt_anim_install(select_prompt_style());
	ret = readline(split_prompt(prompt));
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
		critical_error_errno_ctx("pipe");
	pid = fork();
	if (pid == 0)
	{
		readline_bg_signals();
		close(pp[0]);
		bg_readline(pp[1], prompt, l->edit_mode);
	}
	else if (pid < 0)
		critical_error_errno_ctx("fork");
	else
		return (attach_input_readline(l, pp, pid));
	ft_assert(0);
	return (0);
}
