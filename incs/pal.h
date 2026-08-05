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

#endif
