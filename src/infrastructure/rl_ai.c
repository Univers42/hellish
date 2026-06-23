/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   rl_ai.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/22 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/22 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "rl_private.h"
#include "rl_ghost_ai.h"
#include "ai.h"

/* Wire the AI input features into the active readline keymap: ghost-text
   redisplay, Right-arrow accept of a suggestion, and the idle hook that drives
   async AI suggestions. Called from bg_readline AFTER mascot_install, which
   clears rl_event_hook. */
void	setup_ai_input(void)
{
	rl_redisplay_function = ghost_redisplay;
	rl_bind_keyseq("\\e[C", rl_ghost_accept);
	rl_event_hook = rl_ai_event;
	rl_set_keyboard_input_timeout(100000);
}

/* Readline keybinding (Ctrl-X Ctrl-A): replace the current line with the LLM's
   completion of it. Runs in the forked readline child, so it cannot touch
   t_shell -- ai_complete_line reads its config from the environment. Silent
   no-op when the line is empty or the server is unreachable. The suggestion is
   ft-allocated (parent allocator), so it is freed with xfree, not libc free. */
int	rl_ai_complete(int count, int key)
{
	char	*sug;

	(void)count;
	(void)key;
	if (!rl_line_buffer || !*rl_line_buffer)
		return (0);
	sug = ai_complete_line(rl_line_buffer);
	if (!sug)
		return (0);
	rl_replace_line(sug, 0);
	rl_point = rl_end;
	xfree(sug);
	return (rl_redisplay(), 0);
}
