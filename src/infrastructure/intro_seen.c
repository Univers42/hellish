/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   intro_seen.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/03 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/03 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "header.h"
#include "update.h"
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

/* Build "$HOME/.cache/hellish/seen" into buf. 1 on success. */
static int	seen_path(char *buf, size_t n)
{
	char	*home;

	home = getenv("HOME");
	if (!home || !*home || ft_strlen(home) + 22 >= n)
		return (0);
	ft_strlcpy(buf, home, n);
	ft_strlcat(buf, "/.cache/hellish/seen", n);
	return (1);
}

/* 1 once the entrance animation has played at least once on this machine. */
int	intro_seen(void)
{
	char	path[512];

	if (!seen_path(path, sizeof(path)))
		return (0);
	return (access(path, F_OK) == 0);
}

/* Remember that the animation has played (creating ~/.cache/hellish). */
void	intro_mark_seen(void)
{
	char	path[512];
	char	*home;
	int		fd;

	home = getenv("HOME");
	if (!home || ft_strlen(home) + 22 >= sizeof(path))
		return ;
	ft_strlcpy(path, home, sizeof(path));
	ft_strlcat(path, "/.cache", sizeof(path));
	mkdir(path, 0755);
	ft_strlcat(path, "/hellish", sizeof(path));
	mkdir(path, 0755);
	ft_strlcat(path, "/seen", sizeof(path));
	fd = open(path, O_CREAT | O_WRONLY, 0644);
	if (fd >= 0)
		close(fd);
}
