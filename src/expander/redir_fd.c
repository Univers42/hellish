/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   redir_fd.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/19 20:40:00 by marvin            #+#    #+#             */
/*   Updated: 2026/09/25 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

/* Park a freshly acquired redirection fd out of the low, user-addressable
   range.

   Every redirection of one command is RESOLVED (opened) before any of them
   is APPLIED, and open(2) hands back the lowest free descriptor. That
   descriptor can be the fd a LATER redirection in the same command targets:

       exec 4>a 2>b     open("a") -> 3, open("b") -> 4
                        apply 1:  dup2(3, 4)  <-- overwrites b's fd
                        apply 2:  dup2(4, 2)  <-- fd 2 now points at a

   which left fd 4 closed, fd 2 pointing at `a`, and `b` never written.
   Checking only for a collision with this redirect's OWN src_fd (what the
   code used to do) cannot see that, because the clash is with a sibling
   that has not been resolved yet.

   Parking at >= 10 -- the same range bash reserves for its internal
   descriptors -- makes the scratch fd distinct from every fd 0-9 a script
   can address, so no ordering of redirections can collide. Returns false
   only when the shell is genuinely out of descriptors, in which case the
   original fd is already closed and the redirect must fail. */
bool	redir_park_fd(t_redir *ret)
{
	int	parked;

	if (ret->fd < 0)
		return (false);
	if (ret->fd >= 10)
		return (true);
	parked = fcntl(ret->fd, F_DUPFD, 10);
	close(ret->fd);
	ret->fd = parked;
	return (parked >= 0);
}

/* `>&WORD` is a dup only when WORD is a file DESCRIPTOR (all digits, or the
   `-` that closes one).  `>&out` is not: bash reads it as `&>out`, both
   streams into that file.  Skipping this test was silent -- ft_atoi turned
   `>&out` into fd 0, so the command dup'd stdin onto stdout and the file it
   named was never written at all. */
bool	dup_target_is_fd(const char *fname)
{
	int	i;

	if (fname[0] == '-' && fname[1] == '\0')
		return (true);
	i = 0;
	while (fname[i])
	{
		if (!ft_isdigit((unsigned char)fname[i]))
			return (false);
		i++;
	}
	return (true);
}

/* A redirection onto a descriptor at or above the limit cannot be made:
   dup2 fails with EBADF there, and nowhere else for a valid source. bash
   finds out as it applies it -- after opening the file, which is created
   all the same -- and says "N: Bad file descriptor"; the command does not
   run and the status is 1. This shell applies a command's redirections
   only once all are resolved, past any way back, so the same test is made
   as each one is resolved (`1234567>f` reaches here now that the lexer
   takes every IO_NUMBER whole). A dup is named by its target, as in bash;
   closing a descriptor that cannot be open (`N>&-`) quietly does nothing
   there too. What this resolution opened is closed again on failure, since
   a redirection that failed is never handed to the command's teardown. */
bool	redir_src_fd_ok(t_shell *state, t_redir *r)
{
	long	max;

	max = sysconf(_SC_OPEN_MAX);
	if (r->close_fd || max < 0 || r->src_fd < max)
		return (true);
	if (r->is_dup)
		ft_eprintf("%s: %d: %s\n", state->ctx, r->fd, strerror(EBADF));
	else
		ft_eprintf("%s: %d: %s\n", state->ctx, r->src_fd, strerror(EBADF));
	if (!r->is_dup && r->fd > STDERR_FILENO)
		close(r->fd);
	if (!r->is_dup)
		r->fd = -1;
	return (false);
}
