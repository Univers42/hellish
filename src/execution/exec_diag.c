/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   exec_diag.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/24 18:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/09/24 18:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "execution_private.h"
#include "sys.h"

/* What execve left behind, said the way bash's shell_execve says it (#133).
   Only EACCES used to be reported: a program whose loader or `#!`
   interpreter is missing -- execve's ENOENT on a file that IS there --
   printed nothing at all and exited 127, and a binary for another machine
   was handed to /bin/sh, which printed its own confusion about line 1.
   All of this runs in the forked child, after execve has already failed. */

/* The first line of a sample, the first two when it opens with "#!", holds
   a NUL: bash's check_binary_file. An ELF header is binary outright. */
static bool	sample_is_binary(const unsigned char *s, ssize_t n)
{
	ssize_t	i;
	int		lines;

	if (n >= 4 && s[0] == 0x7f && s[1] == 'E' && s[2] == 'L' && s[3] == 'F')
		return (true);
	lines = 1;
	if (n >= 2 && s[0] == '#' && s[1] == '!')
		lines = 2;
	i = -1;
	while (++i < n)
	{
		if (s[i] == '\n' && --lines == 0)
			return (false);
		if (s[i] == '\0')
			return (true);
	}
	return (false);
}

/* The 80 bytes bash judges a file by. Unreadable or empty is not binary:
   an empty executable is a script that does nothing, status 0. */
bool	exec_file_is_binary(const char *path)
{
	unsigned char	buf[EXEC_SAMPLE_LEN];
	ssize_t			n;
	int				fd;

	fd = open(path, O_RDONLY);
	if (fd < 0)
		return (false);
	n = read(fd, buf, sizeof(buf));
	close(fd);
	return (n > 0 && sample_is_binary(buf, n));
}

/* The interpreter a NUL-terminated "#!" sample names, cut out in place:
   bash's getinterp, a trailing CR spelled ^M -- the classic CRLF script.
   The caller's NUL sits at least one byte past the name, room for the M. */
static char	*sample_interp(char *buf)
{
	int	i;
	int	start;

	i = 2;
	while (buf[i] == ' ' || buf[i] == '\t')
		i++;
	start = i;
	while (buf[i] && buf[i] != '\n' && buf[i] != ' ' && buf[i] != '\t')
		i++;
	if (i > start && buf[i - 1] == '\r')
	{
		buf[i - 1] = '^';
		buf[i++] = 'M';
	}
	buf[i] = '\0';
	return (buf + start);
}

/* A "#!" script the kernel would not start: bash names the interpreter.
   The sample's last byte is dropped as bash drops it. bash says this one
   through sys_error, which never adds "line N:" -- hence the bare dft_ctx
   where every other message here has ctx. */
static bool	bad_interpreter(t_shell *state, char *path, int err)
{
	char	buf[EXEC_SAMPLE_LEN + 1];
	ssize_t	n;
	int		fd;

	fd = open(path, O_RDONLY);
	if (fd < 0)
		return (false);
	n = read(fd, buf, EXEC_SAMPLE_LEN);
	close(fd);
	if (n <= 2 || buf[0] != '#' || buf[1] != '!')
		return (false);
	buf[n - 1] = '\0';
	ft_eprintf("%s: %s: %s: bad interpreter: %s\n", state->dft_ctx, path,
		sample_interp(buf), strerror(err));
	return (true);
}

/* bash's order, and its statuses: 127 only when execve said ENOENT.
     err == EXEC_ERR_BINARY is our own: ENOEXEC on a file the sample says
   is binary, which must never reach the /bin/sh fallback. */
int	exec_failure_report(t_shell *state, char *path, int err)
{
	struct stat	st;

	if (err == EXEC_ERR_BINARY)
		return (ft_eprintf("%s: %s: cannot execute binary file: %s\n",
				state->ctx, path, strerror(ENOEXEC)), EXIT_CMD_NOT_EXEC);
	if (stat(path, &st) == 0 && S_ISDIR(st.st_mode))
		err_2(state, path, strerror(EISDIR));
	else if (access(path, X_OK) != 0 || err == E2BIG || err == ENOMEM)
		err_2(state, path, strerror(err));
	else if (err == ENOENT)
		err_2(state, path, "cannot execute: required file not found");
	else if (!bad_interpreter(state, path, err))
		err_2(state, path, strerror(err));
	if (err == ENOENT)
		return (EXIT_CMD_NOT_FOUND);
	return (EXIT_CMD_NOT_EXEC);
}
