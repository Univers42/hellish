/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   redir_fd.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/19 20:40:00 by marvin            #+#    #+#             */
/*   Updated: 2026/08/19 20:40:00 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"
#include <fcntl.h>
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
