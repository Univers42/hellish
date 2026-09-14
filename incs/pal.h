/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   pal.h                                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/28 16:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/07/28 16:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef PAL_H
# define PAL_H

# include "pal_wait.h"

/* Process shims for shared code; included after shell.h like every other
   header here.  POSIX implementations are thin passthroughs
   (src/platform/posix/pal_proc*.c).  The win32 siblings resolve pids
   through t_shell's pal_procs registry, which is why every wait/kill
   shim carries the shell handle even though POSIX ignores it. */
int		pal_waitpid(t_shell *st, int pid, int *status, int options);
int		pal_wait_any(t_shell *st, int *status, int options);
int		pal_kill(t_shell *st, int pid, int sig);
int		pal_kill_pgid(t_shell *st, int pgid, int sig);
int		pal_pipe(int fds[2]);
void	pal_trap_arm(t_shell *st, int sig, void (*handler)(int));
void	pal_trap_dfl(t_shell *st, int sig);
void	pal_trap_ign(t_shell *st, int sig);

/* The dispositions this shell was started with (src/platform/posix/
   sig_entry.c).  POSIX: a signal ignored on entry cannot be trapped or
   reset -- which is the everyday state of SIGINT and SIGQUIT in any shell
   started as a background job.  Recorded before the shell installs a
   handler of its own, so `trap` can refuse what it must. */
void	pal_entry_signals(void);
void	pal_arm_unwind(int norestart);
int		pal_sig_ignored_on_entry(int sig);
void	seed_entry_traps(t_shell *state);

/* Per-job process groups (src/platform/posix/job_pgrp.c).  Interactive-only
   and self-disabling in forked children; the file header explains why ^Z
   cannot work at all without them. */
void	jc_init(t_shell *st);
void	jc_begin(t_shell *st);
void	jc_child(t_shell *st);
void	jc_parent(t_shell *st, pid_t pid);
void	jc_end(t_shell *st);

#endif
