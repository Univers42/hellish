/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   exit_jobs.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/22 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/22 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"
#include "job_control.h"

/* bash refuses the FIRST `exit` from an interactive shell that still has a
   stopped job, and takes the second one. Without that guard a user who had
   suspended something walked out of the session and left it behind holding
   the terminal -- which is how issue #41 lost a screenful of `top`s on the
   way out. Only interactive shells are guarded: a script or -c that says
   `exit` means it, and gating on INP_RL is also what keeps the golden suite
   (which is entirely non-interactive) from ever seeing this path.

   The "second exit wins" memory is state->exit_warned. bash keys the same
   decision off "the previous command was not the exit builtin"; we clear
   the flag on any other BUILTIN (execute_builtin_cmd_fg) but not after an
   external command, so `exit; ls; exit` leaves on the second exit where
   bash would warn again. Erring toward letting the user out is the right
   side to be wrong on -- the warning has already been shown once. */
bool	exit_stopped_guard(t_shell *state)
{
	int	i;

	if (state->metinp != INP_RL || state->exit_warned)
		return (false);
	i = -1;
	while (++i < JOB_MAX)
	{
		if (state->job_table.jobs[i].pgid != 0
			&& state->job_table.jobs[i].status == JOB_STOPPED)
		{
			ft_eprintf("There are stopped jobs.\n");
			state->exit_warned = true;
			return (true);
		}
	}
	return (false);
}
