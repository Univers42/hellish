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
