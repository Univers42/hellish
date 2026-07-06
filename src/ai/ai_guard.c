/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ai_guard.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/06 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/07/06 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ai.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/sysinfo.h>

/* 1 when the machine is already busy (1-min loadavg >= 3/4 of the cores):
   background AI work (tip refresh) must not pile an inference burst on top of
   a compile or test run -- that is exactly the "commands suddenly take time
   to output" complaint. Reads /proc/loadavg's integer part; a read failure
   reports busy=0 so AI still works on exotic systems. */
int	ai_load_high(void)
{
	char	buf[32];
	int		fd;
	ssize_t	r;

	fd = open("/proc/loadavg", O_RDONLY);
	if (fd < 0)
		return (0);
	r = read(fd, buf, sizeof(buf) - 1);
	close(fd);
	if (r <= 0)
		return (0);
	buf[r] = '\0';
	return (ft_atoi(buf) >= (get_nprocs() * 3) / 4);
}
