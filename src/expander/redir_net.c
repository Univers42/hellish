/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   redir_net.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 19:27:47 by marvin            #+#    #+#             */
/*   Updated: 2026/01/22 19:27:47 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

/* bash's /dev/tcp and /dev/udp virtual redirection targets (issue #16).

   These paths do not exist on any filesystem -- `ls /dev/tcp` fails on a
   stock Linux box -- so a shell that just calls open(2) reports "No such
   file or directory", which is what hellish did.  bash intercepts the path
   before it ever reaches the filesystem and hands back a connected socket
   instead.  ksh does the same; dash does not, so this is an extension, not
   a POSIX requirement.

   The parse is bash's, measured rather than assumed: after the nine-byte
   prefix the host runs to the NEXT slash and the service is everything
   after it, slashes included.  That is why `/dev/tcp/1.2.3.4/80/x` reports
   a bad service name rather than a bad path, and why `/dev/tcp/1.2.3.4`
   with no service at all is not a network path -- it falls through to the
   normal file open and gets "No such file or directory", exactly as bash
   leaves it.

   The service goes to getaddrinfo as a string, so names work as well as
   numbers ("/dev/tcp/host/http"), again matching bash. A resolver failure
   sets EINVAL because the caller's error path prints strerror(errno), and
   EINVAL is what bash ends up reporting for the same input. */

#include "expander_private.h"
#include <sys/socket.h>
#include <netdb.h>

/* SOCK_STREAM for /dev/tcp/, SOCK_DGRAM for /dev/udp/, -1 for anything
   else.  Only the prefix is judged here; whether the rest is well formed
   is net_redir_open's problem. */
static int	net_socktype(const char *fname)
{
	if (ft_strncmp(fname, "/dev/tcp/", 9) == 0)
		return (SOCK_STREAM);
	if (ft_strncmp(fname, "/dev/udp/", 9) == 0)
		return (SOCK_DGRAM);
	return (-1);
}

/* Copy the host out of "host/service" into buf and answer with a pointer
   to the service.  NULL means "no slash after the host", i.e. not a
   network path at all -- the caller must then let the normal file open
   have it, because that is the case bash reports as a missing file. */
static const char	*net_split(const char *rest, char *buf, size_t n)
{
	size_t	len;

	len = 0;
	while (rest[len] && rest[len] != '/')
		len++;
	if (rest[len] != '/' || len >= n)
		return (NULL);
	ft_memcpy(buf, rest, len);
	buf[len] = '\0';
	return (rest + len + 1);
}

/* Resolve and connect.  getaddrinfo can hand back several addresses (a
   host with both A and AAAA records, say); we try each in turn and keep
   the first that connects, which is what makes "localhost" work on a box
   where ::1 is listening and 127.0.0.1 is not, or the reverse. */
static int	net_connect(const char *host, const char *serv, int socktype)
{
	struct addrinfo		hints;
	struct addrinfo		*list;
	struct addrinfo		*ai;
	int					fd;

	ft_memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = socktype;
	if (getaddrinfo(host, serv, &hints, &list) != 0)
		return (errno = EINVAL, -1);
	ai = list;
	while (ai)
	{
		fd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
		if (fd >= 0 && connect(fd, ai->ai_addr, ai->ai_addrlen) == 0)
			return (freeaddrinfo(list), fd);
		if (fd >= 0)
			close(fd);
		ai = ai->ai_next;
	}
	return (freeaddrinfo(list), -1);
}

/* -1 = not a network path, leave it to the file open; 0 = it was one and
   it failed (errno is set, and the caller prints strerror for it); 1 = a
   connected socket is in ret->fd.

   The fd shuffle at the end is the same one open_file_redir needs and for
   the same reason: socket(2) hands back the lowest free descriptor, which
   can be the very fd this redirect targets, and the apply step would then
   be a no-op that the teardown promptly closes. */
int	net_redir_open(char *fname, t_redir *ret)
{
	char			host[256];
	const char		*serv;
	int				socktype;

	socktype = net_socktype(fname);
	if (socktype < 0)
		return (-1);
	serv = net_split(fname + 9, host, sizeof(host));
	if (!serv)
		return (-1);
	ret->fd = net_connect(host, serv, socktype);
	if (ret->fd < 0)
		return (0);
	if (!redir_park_fd(ret))
		return (0);
	ret->should_delete = false;
	return (1);
}
