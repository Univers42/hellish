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

/* Enter the editor for one read: publish the shell to the widgets, draw
   the prompt's upper rows ourselves, arm the right-prompt painter and the
   animation. Returns the one row readline itself is handed. */
char	*rl_editor_enter(t_shell *state, char *prompt)
{
	char	*row;

	zle_enter(state);
	debug_dump_prompt(prompt);
	row = split_prompt(prompt);
	state->rl.rp_prompt_w = visible_width_cstr(row);
	rl_rprompt_install(state);
	return (row);
}

/* Leave it: the `zle` builtin refuses from here on, and nothing the
   editor installed for this read outlives it. */
void	rl_editor_exit(void)
{
	zle_enter(NULL);
	rl_redisplay_function = rl_redisplay;
}

/* Read one line of interactive input into state->rl.buff. 0 = a line,
   1 = EOF, 2 = interrupted. readline is brought up to date first (its
   one-time setup, new bindings, the editing mode). */
int	get_more_input_readline(t_shell *state, char *prompt)
{
	rl_preinit(&state->rl);
	return (rl_read_fork(state, prompt));
}
