/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtin_ai_setup2.c                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/22 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/22 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>

/* Fill buf with $HOME/.hellishrc. NULL if $HOME is unset. */
static char	*rc_path(char *buf, size_t n)
{
	char	*home;

	home = getenv("HOME");
	if (!home)
		return (NULL);
	ft_strlcpy(buf, home, n);
	ft_strlcat(buf, "/.hellishrc", n);
	return (buf);
}

/* Read ~/.hellishrc whole (xfree it); empty string if absent.
   ponytail: 64 KB cap -- a dotfile, never larger in practice. */
char	*ai_rc_read(void)
{
	char	path[1024];
	char	*buf;
	int		fd;
	ssize_t	r;
	size_t	len;

	if (!rc_path(path, sizeof(path)))
		return (ft_strdup(""));
	fd = open(path, O_RDONLY);
	if (fd < 0)
		return (ft_strdup(""));
	buf = xmalloc(65536);
	len = 0;
	r = read(fd, buf, 65535);
	while (r > 0 && len < 65535)
	{
		len += (size_t)r;
		r = read(fd, buf + len, 65535 - len);
	}
	close(fd);
	buf[len] = '\0';
	return (buf);
}

/* Overwrite ~/.hellishrc with content. 0 on success, -1 on failure. */
int	ai_rc_write(const char *content)
{
	char	path[1024];
	int		fd;

	if (!rc_path(path, sizeof(path)))
		return (-1);
	fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (fd < 0)
		return (-1);
	if (write(fd, content, ft_strlen(content)) < 0)
		return (close(fd), -1);
	close(fd);
	return (0);
}

/* Read one line from stdin with echo OFF (for an API key), newline trimmed. */
void	read_secret(char *buf, size_t n)
{
	struct termios	old;
	struct termios	raw;
	ssize_t			r;

	if (tcgetattr(STDIN_FILENO, &old) == 0)
	{
		raw = old;
		raw.c_lflag &= ~ECHO;
		tcsetattr(STDIN_FILENO, TCSANOW, &raw);
	}
	r = read(STDIN_FILENO, buf, n - 1);
	tcsetattr(STDIN_FILENO, TCSANOW, &old);
	if (r < 0)
		r = 0;
	buf[r] = '\0';
	while (r > 0 && (buf[r - 1] == '\n' || buf[r - 1] == '\r'))
		buf[--r] = '\0';
	ft_eprintf("\n");
}
