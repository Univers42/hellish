/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   history_fd.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/25 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/09/25 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "history_private.h"
#include <sys/stat.h>

/* The descriptor this session streams its history into.
**
** It shares the fd table with the user's own descriptors, and it used to
** be opened the plain way: the lowest free number, 3, inherited by every
** program the shell ran (`ls -l /proc/self/fd` listed the history file),
** and sitting in the 3..9 range scripts claim for themselves. After
** `exec 3>f` the history went on into f -- the user's file -- and after
** `exec 3>&-` it went nowhere for the rest of the session.
**
** So it is opened close-on-exec, moved out of that range the way save_fd
** moves the executor's copies, and remembered by identity (device and
** inode) as well as by number: a number the user has since redirected or
** closed is theirs, never written to or closed by the shell, and the
** history file is simply opened again. */

/* Open path for appending, as the session's history descriptor. On any
   failure append_fd is -1 and the session records in memory only. */
void	hist_open_append(t_shell *state, const char *path)
{
	int			fd;
	int			high;
	struct stat	st;

	state->hist.append_fd = -1;
	fd = open(path, O_CREAT | O_WRONLY | O_APPEND | O_CLOEXEC, 0600);
	if (fd < 0)
		return ;
	high = fcntl(fd, F_DUPFD_CLOEXEC, 10);
	if (high >= 0)
	{
		close(fd);
		fd = high;
	}
	if (fstat(fd, &st) < 0)
	{
		close(fd);
		return ;
	}
	state->hist.append_fd = fd;
	state->hist.append_dev = st.st_dev;
	state->hist.append_ino = st.st_ino;
}

/* True while append_fd still refers to the file it was opened on. */
static bool	hist_fd_ours(t_history *h)
{
	struct stat	st;

	if (h->append_fd < 0 || fstat(h->append_fd, &st) < 0)
		return (false);
	return (st.st_dev == h->append_dev && st.st_ino == h->append_ino);
}

/* The descriptor to append the next entry to, or -1: append_fd while it
   is still the history file, else a fresh one on the same path. One
   fstat per recorded command. */
int	hist_append_fd(t_shell *state)
{
	if (state->hist.append_fd < 0 || hist_fd_ours(&state->hist))
		return (state->hist.append_fd);
	if (!state->hist.file)
	{
		state->hist.append_fd = -1;
		return (-1);
	}
	hist_open_append(state, state->hist.file);
	return (state->hist.append_fd);
}

/* Stop streaming: close append_fd only if the number is still ours. */
void	hist_close_append(t_shell *state)
{
	if (hist_fd_ours(&state->hist))
		close(state->hist.append_fd);
	state->hist.append_fd = -1;
}
