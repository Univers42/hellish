/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   job_hangup.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/23 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/23 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "shell.h"
#include "job_control.h"
#include "sh_input.h"
#include "sys.h"
#include <signal.h>
#include <time.h>

/* Are any of this shell's jobs still alive? */
static int	jobs_still_alive(t_shell *st)
{
	int	i;

	i = -1;
	while (++i < JOB_MAX)
		if (st->job_table.jobs[i].pgid != 0
			&& !job_finished(&st->job_table.jobs[i]))
			return (1);
	return (0);
}

/* SIGCONT before SIGHUP, and that order is not decorative: a STOPPED
   process never runs its handler, so a hangup delivered on its own just
   queues behind the stop and the job sits there. Continue it first and the
   hangup is acted on. */
static void	hangup_every_job(t_shell *st)
{
	int	i;

	i = -1;
	while (++i < JOB_MAX)
	{
		if (st->job_table.jobs[i].pgid != 0
			&& !job_finished(&st->job_table.jobs[i]))
		{
			kill(-st->job_table.jobs[i].pgid, SIGHUP);
			kill(-st->job_table.jobs[i].pgid, SIGCONT);
		}
	}
}

/* Sleep a few milliseconds without pulling in usleep(). */
static void	nap_ms(long ms)
{
	struct timespec	ts;

	ts.tv_sec = ms / 1000;
	ts.tv_nsec = (ms % 1000) * 1000000L;
	nanosleep(&ts, NULL);
}

/* Take the background jobs down BEFORE the terminal is handed back, and
   wait for them to actually go.

   The waiting is the whole point, and skipping it is what issue #58 looked
   like even after the exit guard was fixed. A full-screen program that is
   hung up does not die silently: it repaints, restores its own idea of the
   terminal and prints a farewell newline on the way out. With 25 stopped
   `top`s that is 25 processes all writing to the tty. If the shell restores
   the terminal and exits first, every one of those writes lands AFTER the
   restore -- on a terminal that now belongs to the parent shell -- and the
   user is left with a screenful of blank lines and their next command
   spliced into the debris.

   So: hang them up, drain them, and only then does off() put the terminal
   back. The wait is bounded (two seconds in 10 ms steps) because exiting a
   shell must never hang on a process that refuses to die; whatever is left
   after that is reparented to init and no longer our problem.

   Interactive shells only. A script's background jobs are deliberately left
   to outlive it -- `cmd & disown` style fire-and-forget is a documented
   thing to do in a script, and nothing there is holding a terminal. */
void	jobs_hangup_on_exit(t_shell *st)
{
	int	spins;

	if (st->metinp != INP_RL || !jobs_still_alive(st))
		return ;
	hangup_every_job(st);
	spins = 0;
	while (spins++ < 200 && jobs_still_alive(st))
	{
		job_update_status(st);
		nap_ms(10);
	}
}
