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
#include "zle.h"
#include <locale.h>

void	setup_completion(void);
void	setup_vi_mode(void);
void	setup_emacs_mode(void);

/* Print every line of `prompt` except the last to rl_outstream, stripping the
   \001/\002 width markers (they must not reach the terminal). Returns the final
   line. readline only ever sees a single-line prompt, which avoids the cursor
   drift and heavy full-line redraws that a multi-line prompt (embedded \n)
   causes during ↑/↓ history navigation.
   The whole prefix goes out in ONE write(). It used to stream through
   unbuffered fputc on stderr -- a write() syscall PER BYTE -- and the tty
   line discipline echoes type-ahead between two user-space writes. A key
   pressed here therefore landed inside a colour escape, and since every
   letter is a valid CSI final byte the sequence ended early and its tail
   printed as literal text (`38;2;112`, `;79;87m`). Same reason
   mascot_redraw composes its frame in memory first. */
static char	*split_prompt(char *prompt)
{
	t_string	f;
	char		*nl;
	size_t		i;

	nl = ft_strrchr(prompt, '\n');
	if (!nl)
		return (prompt);
	vec_init(&f);
	f.elem_size = 1;
	i = 0;
	while (prompt + i <= nl)
	{
		if (prompt[i] != '\001' && prompt[i] != '\002')
			vec_push_char(&f, prompt[i]);
		i++;
	}
	tty_write_all(fileno(rl_outstream), f.ctx, f.len);
	xfree(f.ctx);
	return (nl + 1);
}

/* Dump the raw prompt bytes and computed visible width to stderr when the env
   var MINISHELL_DEBUG_PROMPT is set. Handy for chasing cursor-drift bugs where
   a missing \001/\002 bracket is expanding the width by N ESC bytes. */
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

/* The readline-in-a-forked-child trick: we cannot call readline in the parent
   because it installs global signal handlers and terminal state that would
   corrupt the parent shell. fork → child calls readline → writes over a pipe
   → exits. stdin/stdout are inherited; rl_outstream is redirected to stderr
   so readline's display uses the right fd. Exit 0 = line, 1 = EOF (^D). One
   trap: readline's buffer is libc-malloc'd, so free(ret) uses libc free, not
   xfree -- at SAFE=0 that would hit the ft_malloc heap and corrupt it.
     zle_install goes AFTER the editing mode is chosen: setup_emacs_mode and
   setup_vi_mode replace the keymap, so a binding installed before them
   would be discarded and the key would silently do nothing. */
void	bg_readline(int outfd, char *prompt, int edit_mode, t_shell *state)
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
	zle_install(state);
	debug_dump_prompt(prompt);
	mascot_install();
	ret = readline(split_prompt(prompt));
	if (!ret)
		(close(outfd), exit (1));
	(write_to_file(ret, outfd), free(ret), close(outfd), exit(0));
}

/* Parent side of the fork: drain the pipe into the buffer and wait for the
   child. If the child was killed by a signal (e.g. SIGINT in readline),
   return 2 to propagate the interrupt; otherwise use the child's exit status
   (0 = line, 1 = EOF). The waitpid loop retries on EINTR.
   The animation frames are single-shot: the fork that just happened gave
   the child its own armed copy, so the parent's cells are disarmed here.
   A continuation read (dquote> / heredoc> / loop body) forks again WITHOUT
   a ps1_animated render in between -- the rows above its cursor are the
   user's earlier input lines, and an armed hook would stamp the PS1 info
   row over them once per tick. Only the next PS1 render re-arms. */
int	attach_input_readline(t_rl *l, int pp[2], int pid)
{
	int	status;

	anim_cells()->count = 0;
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

/* Entry point for the readline fork dance: create the pipe, fork, and hand
   each side to its respective function. The child never returns (bg_readline
   always _exit()s); ft_assert(0) below is just a belt-and-suspenders guard in
   case the compiler does not see that. */
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
		bg_readline(pp[1], prompt, l->edit_mode, zle_caller());
	}
	else if (pid < 0)
		critical_error_errno_ctx("fork");
	else
		return (attach_input_readline(l, pp, pid));
	ft_assert(0);
	return (0);
}
