/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   rl_keyfd.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/25 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/09/25 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "rl_private.h"
#include <fcntl.h>
#include <sys/stat.h>

/* The terminal the line is read from, opened a second time to read the
** keys from: an open file description of its own, O_NONBLOCK, so that a
** read of it can never block.
**
** rl_getc_hook waits in pselect, which lets a signal in atomically, and
** reads once pselect reports a key. A ^C can be processed between the
** two, and the line discipline then does two things in this order: it
** sends SIGINT -- readline's handler runs before read() is entered, so
** there is no EINTR to come -- and it flushes the input queue, taking the
** key pselect saw. A blocking read() slept on the empty queue until the
** NEXT key, and gave that key to the line the ^C had ended: ^C in a ^R
** search, then `echo ...` ran as `cho ...` (rl_read_race_test.py). From
** here the flushed key is EAGAIN, and the loop goes back to pselect,
** which reports the signal.
**
** O_NONBLOCK is not set on fd 0 itself: that description is shared with
** every program the shell runs, and #34 is what a non-blocking terminal
** does to them. It sits at fd 255, where bash keeps its terminal, close on
** exec, and is checked at every prompt (rl_preinit): when fd 0 is no
** longer that terminal, or the slot shows something else (a redirection
** onto 255 closed ours), it is opened again. A terminal that cannot be
** opened again -- after `su`, the device is not ours -- is read through
** fd 0, as before.
**
** fd 0 is put back to blocking at every prompt too, as bash does before
** every readline(): a program that left it non-blocking would otherwise
** hand the next command an EAGAIN on stdin. */

static t_rl_keyfd	*keyfd_cell(void)
{
	static t_rl_keyfd	k = {-1, -1, 0};

	return (&k);
}

/* The descriptor to read the keys of `fd` from. */
int	rl_keyfd(int fd)
{
	t_rl_keyfd	*k;

	k = keyfd_cell();
	if (k->twin >= 0 && k->src == fd)
		return (k->twin);
	return (fd);
}

/* The second descriptor is still open and still the terminal `rdev`. */
static bool	keyfd_is(t_rl_keyfd *k, dev_t rdev)
{
	struct stat	st;

	return (k->twin >= 0 && fstat(k->twin, &st) == 0
		&& S_ISCHR(st.st_mode) && st.st_rdev == rdev);
}

/* Open the terminal on `fd` again, non-blocking, parked at fd 255 or the
   first free one above (10 and up where the limit is lower). */
static void	keyfd_open(t_rl_keyfd *k, int fd, dev_t rdev)
{
	char	*path;
	int		nfd;
	int		high;

	path = ttyname(fd);
	if (!path)
		return ;
	nfd = open(path, O_RDONLY | O_NONBLOCK | O_NOCTTY | O_CLOEXEC);
	if (nfd < 0)
		return ;
	high = fcntl(nfd, F_DUPFD_CLOEXEC, 255);
	if (high < 0)
		high = fcntl(nfd, F_DUPFD_CLOEXEC, 10);
	if (high >= 0)
	{
		close(nfd);
		nfd = high;
	}
	k->twin = nfd;
	k->src = fd;
	k->rdev = rdev;
}

/* Before a read of `fd`: fd 0 blocking again, and the second descriptor
   made to match it. An old one is closed only while it still is the
   terminal it was opened on: a slot that shows anything else was taken
   over by the user, and that descriptor is theirs now. */
void	rl_keyfd_refresh(int fd)
{
	t_rl_keyfd	*k;
	struct stat	st;
	bool		tty;
	int			fl;

	k = keyfd_cell();
	fl = fcntl(fd, F_GETFL);
	if (fl >= 0 && (fl & O_NONBLOCK))
		fcntl(fd, F_SETFL, fl & ~O_NONBLOCK);
	tty = (fstat(fd, &st) == 0 && S_ISCHR(st.st_mode));
	if (tty && k->src == fd && keyfd_is(k, st.st_rdev))
		return ;
	if (keyfd_is(k, k->rdev))
		close(k->twin);
	k->twin = -1;
	k->src = -1;
	if (tty)
		keyfd_open(k, fd, st.st_rdev);
}
