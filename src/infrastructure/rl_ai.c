/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   rl_ai.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/22 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/07/06 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "rl_private.h"
#include "rl_ghost_ai.h"
#include "ai.h"

/* Signal event hook: readline calls this when a signal interrupts an input
   read (our SIGWINCH handler installs without SA_RESTART exactly so read()
   EINTRs). With the event hook active readline mostly waits in select(), so
   the idle tick services most resizes; this covers the read() path. */
static int	rl_resize_event(void)
{
	if (rl_resize_fixup())
		rl_forced_update_display();
	return (0);
}

/* getc wrapper: the moment ANY key arrives, wipe a painted ghost BEFORE
   readline processes the key and repaints. This is what lets the ghost live
   with readline's DEFAULT redisplay (no stale bytes to confuse its diff) --
   and it erases an abandoned suggestion on Enter for free. */
static int	ghost_getc(FILE *stream)
{
	int	c;

	c = rl_getc(stream);
	ghost_erase_pending();
	return (c);
}

/* Right-arrow: accept the whole suggestion when one is showing, otherwise the
   normal forward-char. (The ghost was just erased by the getc wrapper; its
   text is recomputed from memory, not read back from the screen.) */
int	rl_ghost_accept(int count, int key)
{
	const char	*g;

	g = ghost_suffix();
	if (g && *g)
		return (rl_insert_text((char *)g), 0);
	return (rl_forward_char(count, key));
}

/* Wire the AI input features into the active readline keymap: the ghost-
   erasing getc wrapper, Right-arrow accept, Ctrl-X Ctrl-A full completion,
   and the idle hook that paints ghosts and drives async AI suggestions.
   readline's DEFAULT redisplay stays installed -- replacing it silently
   degrades multi-line rendering (recalled multi-line history turns into ^J
   soup). Called from bg_readline AFTER the vi/emacs keymap switch (so the
   binds land in the live keymap) and AFTER mascot_install, which clears
   rl_event_hook. */
void	setup_ai_input(void)
{
	rl_getc_function = ghost_getc;
	rl_bind_keyseq("\\e[C", rl_ghost_accept);
	rl_bind_keyseq("\\C-x\\C-a", rl_ai_complete);
	rl_event_hook = rl_ai_event;
	rl_signal_event_hook = rl_resize_event;
	rl_set_keyboard_input_timeout(100000);
	rl_resize_setup();
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
