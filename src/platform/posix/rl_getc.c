/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   rl_getc.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/17 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/09/17 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "rl_private.h"
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/select.h>

/* readline's byte source (rl_getc_function), in place of its own rl_getc.
**
** readline's rl_getc waits in pselect and, when a signal interrupts it,
** hands the signal to readline's cleanup, calls rl_signal_event_hook and
** goes straight back to waiting. An application reading in its own
** process has no way to end the read on ^C there short of longjmp, which
** is what bash does. This one can: a SIGINT that readline handled during
** the read, or any other reason to stop (rl_should_abort), makes it
** return READERR, readline's own "stop reading" answer (readline.c), and
** readline() then returns NULL.
**
** It is also where the prompt's idle work runs -- the mascot animation
** ticks on a timeout -- since an rl_event_hook would make readline stop
** calling us at all (it reads through rl_gather_tyi instead, which also
** swallows typeahead into its own buffer).
**
** One byte per read(2): queued Enters stay in the terminal, and each is
** read by the prompt that comes up for it. The read is of the terminal's
** second, non-blocking descriptor where there is one (rl_keyfd.c), so the
** only place this ever waits is pselect. */

/* A signal readline caught since the last look: let readline handle it
   now -- cleanup, the application's handler, re-prep -- and remember an
   interrupt, which ends this read. */
static void	rl_take_signals(void)
{
	int	sig;

	sig = rl_pending_signal();
	if (!sig)
		return ;
	if (sig == SIGINT)
		*rl_intr_cell() = 1;
	rl_check_signals();
}

static struct timespec	*rl_timeout(struct timespec *ts)
{
	int	ms;

	ms = rl_idle_timeout();
	if (ms < 0)
		return (NULL);
	ts->tv_sec = ms / 1000;
	ts->tv_nsec = (ms % 1000) * 1000000L;
	return (ts);
}

/* Wait for the terminal (or the idle fd), with `old` -- the mask from
   before rl_block -- in force only while waiting, so a signal cannot slip
   in between the last check and the wait. */
static int	rl_wait(int fd, sigset_t *old, fd_set *set)
{
	struct timespec	ts;
	int				idle;
	int				nfds;

	FD_ZERO(set);
	FD_SET(fd, set);
	nfds = fd + 1;
	idle = rl_idle_fd();
	if (idle >= 0)
		FD_SET(idle, set);
	if (idle >= nfds)
		nfds = idle + 1;
	return (pselect(nfds, set, NULL, NULL, rl_timeout(&ts), old));
}

/* One byte, or EOF, or RL_AGAIN when the read should simply be retried.
   From the second descriptor (rl_keyfd.c) EAGAIN means the key pselect saw
   is gone -- a ^C flushed it -- and pselect is where to wait now. fd 0
   itself, left non-blocking by some program, is put back, as readline's
   rl_getc does. Any other error ends the read the way readline's does. */
static int	rl_read1(int fd, bool twin)
{
	unsigned char	c;
	ssize_t			n;

	n = read(fd, &c, 1);
	if (n == 1)
		return (c);
	if (n == 0)
		return (EOF);
	if (errno == EINTR)
		return (RL_AGAIN);
	if (errno == EAGAIN || errno == EWOULDBLOCK)
	{
		if (!twin && fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) & ~O_NONBLOCK) < 0)
			return (EOF);
		return (RL_AGAIN);
	}
	if (RL_ISSTATE(RL_STATE_READCMD))
		return (READERR);
	return (EOF);
}

int	rl_getc_hook(FILE *stream)
{
	sigset_t	old;
	fd_set		set;
	int			fd;
	int			r;

	fd = rl_keyfd(fileno(stream));
	r = RL_AGAIN;
	while (r == RL_AGAIN)
	{
		rl_take_signals();
		if (rl_should_abort())
			return (rl_abort_value());
		rl_block(&old);
		r = -1;
		if (!rl_pending_signal() && !rl_should_abort())
			r = rl_wait(fd, &old, &set);
		sigprocmask(SIG_SETMASK, &old, NULL);
		if (r > 0 && FD_ISSET(fd, &set))
			r = rl_read1(fd, fd != fileno(stream));
		else
			r = (rl_idle_event(r), RL_AGAIN);
	}
	return (r);
}
