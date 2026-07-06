/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ai_net.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/22 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/22 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ai.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <errno.h>

/* Bound both directions so a stalled server can never hang the shell. */
static void	set_timeout(int fd, int ms)
{
	struct timeval	tv;

	tv.tv_sec = ms / 1000;
	tv.tv_usec = (ms % 1000) * 1000;
	setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
	setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
}

/* Bound a non-blocking connect(): wait up to timeout_ms for writability, then
   confirm via SO_ERROR. SO_*TIMEO does NOT cover connect(), so without this a
   blackholed host hangs the shell on SYN retries (~127s). 0 on success. */
static int	wait_connect(int fd, int timeout_ms)
{
	struct pollfd	pfd;
	int				err;
	socklen_t		len;

	pfd.fd = fd;
	pfd.events = POLLOUT;
	if (poll(&pfd, 1, timeout_ms) <= 0)
		return (-1);
	len = sizeof(err);
	if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &len) < 0 || err != 0)
		return (-1);
	return (0);
}

/* Connect a TCP socket to host:port with a real timeout on connect itself, then
   restore blocking mode so set_timeout bounds the later send/recv. */
int	ai_connect(const char *host, int port, int timeout_ms)
{
	int					fd;
	struct sockaddr_in	addr;
	int					fl;

	fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0)
		return (-1);
	ft_bzero(&addr, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons((uint16_t)port);
	if (inet_pton(AF_INET, host, &addr.sin_addr) != 1)
		return (close(fd), -1);
	fl = fcntl(fd, F_GETFL, 0);
	fcntl(fd, F_SETFL, fl | O_NONBLOCK);
	if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0
		&& (errno != EINPROGRESS || wait_connect(fd, timeout_ms) < 0))
		return (close(fd), -1);
	fcntl(fd, F_SETFL, fl);
	set_timeout(fd, timeout_ms);
	return (fd);
}

int	ai_send_all(int fd, const char *buf, size_t len)
{
	ssize_t	n;
	size_t	off;

	off = 0;
	while (off < len)
	{
		n = send(fd, buf + off, len - off, 0);
		if (n <= 0)
			return (-1);
		off += (size_t)n;
	}
	return (0);
}

/* Read until EOF (the server sends Connection: close), into a capped buffer.
   Returns a fresh NUL-terminated string. */
char	*ai_read_all(int fd)
{
	char	*buf;
	size_t	len;
	ssize_t	n;

	buf = xmalloc(AI_MAX_REPLY + 1);
	len = 0;
	n = read(fd, buf + len, AI_MAX_REPLY - len);
	while (n > 0 && len < AI_MAX_REPLY)
	{
		len += (size_t)n;
		n = read(fd, buf + len, AI_MAX_REPLY - len);
	}
	buf[len] = '\0';
	return (buf);
}
