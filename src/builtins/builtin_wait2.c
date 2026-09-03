/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtin_wait2.c                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/03 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/09/03 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"
#include "job_control.h"
#include <sys/wait.h>

void	bg_done_record(t_shell *state, pid_t pid, int status);
int		bg_done_take(t_shell *state, pid_t pid, int *status);

/* A bare `wait`, with bash's accounting of what it reported.
**
** bash reaps in its SIGCHLD handler, so a job that died a moment ago is
** already dead-and-reaped when `wait` begins, and `wait` leaves it alone:
** the next `jobs` prints its Done/Terminated line. Only the jobs that die
** WHILE `wait` blocks are `wait`'s own, and those it retires. hellish
** reaps between commands instead, and a `-c` string is one command list,
** so `sleep 0.05 & sleep 0.2; wait; jobs` reached `wait` with the child
** dead but unreaped -- `wait` reaped it, counted it as reported, and
** `jobs` had nothing to say where bash prints Done. The arm64 rung sat
** on that side of the race every other push (issue27_job_pgrp).
**
** So `wait` first drains what is already dead without blocking -- that is
** the SIGCHLD handler's work, done late, and counts as "died before" --
** and only then blocks; what arrives during the block is marked reported
** and retired at the end. */
static void	wait_drain(t_shell *state, int flags, bool report)
{
	int		status;
	int		drop;
	pid_t	pid;

	pid = pal_wait_any(state, &status, flags);
	while (pid > 0)
	{
		bg_done_record(state, pid, status);
		bg_done_take(state, pid, &drop);
		if (report)
			job_mark_reported(&state->job_table, pid);
		pid = pal_wait_any(state, &status, flags);
	}
}

/* Interactive shells retire nothing here: bash prints "[1]+ Done" at the
   next prompt for a job that ended inside `wait` as much as for one that
   ended before it, and job_notify is what prints those. In a script:
   drain what is already dead (the handler's work, done late), wait for
   the jobs in order, then mark every dead job reported except `$!`'s --
   bash's mark_dead_jobs_as_notified skips last_asynchronous_pid -- and
   retire what is reported. */
/* bash waits for its jobs one at a time, in table order, and only the job
   it is blocked on when it dies is reaped BY `wait`: a later job that
   ends meanwhile is dead-before-its-turn, like one the handler reaped,
   and stays for `jobs`. So `sleep 0.3 & sleep 0.05 & wait; jobs` prints
   job 2's Done line -- it ended while `wait` was busy with job 1. */
static void	wait_in_order(t_shell *state)
{
	t_job	*head;
	pid_t	pid;
	int		status;
	int		drop;

	head = job_first_unfinished(&state->job_table);
	while (head)
	{
		pid = pal_wait_any(state, &status, 0);
		if (pid <= 0)
			return ;
		bg_done_record(state, pid, status);
		bg_done_take(state, pid, &drop);
		if (pid == head->pgid)
			job_mark_reported(&state->job_table, pid);
		head = job_first_unfinished(&state->job_table);
	}
}

int	wait_all(t_shell *state)
{
	pid_t	keep;

	if (state->metinp == INP_RL)
	{
		wait_drain(state, 0, false);
		return (0);
	}
	wait_drain(state, WNOHANG, false);
	wait_in_order(state);
	wait_drain(state, 0, false);
	keep = 0;
	if (state->last_bg_pid)
		keep = (pid_t)ft_atoi(state->last_bg_pid);
	job_mark_reported_except(&state->job_table, keep);
	job_purge_reported(&state->job_table);
	return (0);
}
