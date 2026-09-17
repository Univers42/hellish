/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   rl_inproc.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/17 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/09/17 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "rl_private.h"
#include "zle.h"

/* The line, read in the shell process.
**
** Every line used to be read in a forked child: readline installs signal
** handlers and puts the terminal in raw mode, and the fork kept both away
** from the shell. That cost a process per prompt, and it made the prompt
** a race between two processes for the same terminal. The containment is
** now done in place (rl_getc.c ends a read on ^C, rl_editor.c brackets
** shell code run from the editor, readline itself restores the terminal
** on the signals it catches), and the child is gone: a bare Enter creates
** no process.
**
** HELLISH_RL_FORK=1 in the environment brings the child back for a
** release (rl_fork.c), in case this path misbehaves somewhere. */

/* The line readline returned becomes the shell's input. readline's buffer
   comes from libc malloc: free(), never xfree, which at SAFE=0 would hand
   it to the ft_malloc heap. vec_push_nstr leaves the buffer terminated,
   as vec_append_fd did for the forked reader. */
static void	rl_line_take(t_shell *state, char *line)
{
	vec_push_nstr(&state->rl.buff, line, ft_strlen(line));
	free(line);
	buff_readline_update(&state->rl);
}

/* After ^C the row has to end, as the forked reader's parent ended it.
   readline does it itself when bracketed paste is on: the read ended the
   way an EOF does, and rl_deprep_terminal prints a newline after
   switching bracketed paste off (rltty.c). Otherwise it is ours. */
static void	rl_intr_newline(void)
{
	const char	*bp;

	bp = rl_variable_value("enable-bracketed-paste");
	if (!bp || ft_strcmp(bp, "on") != 0)
		ft_eprintf("\n");
}

/* 0 = a line, 1 = EOF, 2 = interrupted. The animation frames served this
   read only, as they served one child. */
int	rl_read_inproc(t_shell *state, char *prompt)
{
	char	*row;
	char	*line;

	row = rl_editor_enter(state, prompt);
	line = readline(row);
	rl_editor_exit(state);
	anim_cells()->count = 0;
	if (*rl_intr_cell() || get_g_sig()->should_unwind)
	{
		free(line);
		rl_intr_newline();
		return (2);
	}
	if (!line)
		return (1);
	rl_line_take(state, line);
	return (0);
}
