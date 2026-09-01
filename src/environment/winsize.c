/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   winsize.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/01 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/09/01 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "env.h"
#include "sh_input.h"
#include <sys/ioctl.h>
#include <unistd.h>

/* Refresh one of COLUMNS/LINES. A plain shell variable, as in bash:
   children measure their own terminal, so the value must NOT leak into
   execve's envp — unless the user exported it, in which case the update
   keeps the export (env_set would otherwise clobber the flag). */
static void	set_winsize_var(t_shell *state, const char *key, int val)
{
	t_env	*old;
	bool	exported;

	old = env_get(&state->env, (char *)key);
	exported = (old && old->exported);
	env_set(&state->env, env_create(ft_strdup((char *)key),
			ft_itoa(val), exported));
}

/* COLUMNS and LINES for interactive shells (issue #97). bash sets both at
   startup and keeps them current (checkwinsize, default-on since 5.0);
   without them every prompt that right-aligns has to fork `tput cols`.
   Called at session start and again before each top-level execution, so
   a resize — the kernel updates the pty size whether or not anyone
   catches SIGWINCH — is visible to the very next command, which is the
   strongest observable guarantee bash offers. Non-interactive shells set
   neither, exactly like bash. */
void	update_winsize_vars(t_shell *state)
{
	struct winsize	ws;

	if (state->metinp != INP_RL)
		return ;
	if (ioctl(STDERR_FILENO, TIOCGWINSZ, &ws) != 0
		|| ws.ws_col <= 0 || ws.ws_row <= 0)
		return ;
	set_winsize_var(state, "COLUMNS", ws.ws_col);
	set_winsize_var(state, "LINES", ws.ws_row);
}
