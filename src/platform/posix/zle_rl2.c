/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   zle_rl2.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/30 15:40:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/30 15:40:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "shell.h"
#include "libft.h"
#include "zle.h"
#include <readline/readline.h>
#include <stdio.h>

int	exec_string(t_shell *state, char *content);

/* The built-in widgets, and the install step that hands the recorded
   bindings to readline.
     These live on the platform side because each is one readline call and
   nothing else: putting them behind an abstraction would be a layer with
   one implementation. */

bool	zle_active(void)
{
	return (*zle_state_cell() != NULL);
}

void	zle_do_redisplay(void)
{
	rl_redisplay();
}

/* zle -M: the message on a line of its own, and the edited line painted
** again underneath it.
**
** rl_on_new_line_with_prompt is the call that is easy to leave out and
** impossible to miss once it is missing: readline caches where it believes
** the cursor is, and text written to the terminal behind its back
** invalidates that cache. Skip the re-anchor and the next keystroke
** repaints over the message, or over the user's own line, depending on how
** long each is -- which reads as a rendering race rather than one absent
** call.
*/
void	zle_do_message(const char *msg)
{
	if (!msg)
		return ;
	rl_crlf();
	fputs(msg, rl_outstream);
	rl_crlf();
	fflush(rl_outstream);
	rl_on_new_line_with_prompt();
	rl_forced_update_display();
}

void	zle_do_kill_buffer(void)
{
	rl_replace_line("", 0);
	rl_point = 0;
}

/* accept-line: submit the line as it stands.  rl_done is readline's own
   "stop reading" flag, so the line goes back through the same path a
   pressed Return takes -- there is no second submission mechanism that
   could disagree with it about trailing state. */
void	zle_do_accept_line(void)
{
	rl_done = 1;
}
