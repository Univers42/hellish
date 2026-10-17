/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   complete_make_read.c                               :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/16 18:30:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/09/16 18:30:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

/* Reading a Makefile and cutting it into LOGICAL lines (a backslash-
   newline becomes one space, as in GNU make; CRLF undone), then
   handing each to mk_line. */

#include "complete_make.h"
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

static size_t	mk_fill_buf(int fd, char *buf, size_t cap)
{
	size_t	got;
	ssize_t	r;

	got = 0;
	r = 1;
	while (r > 0 && got < cap)
	{
		r = read(fd, buf + got, cap - got);
		if (r > 0)
			got += (size_t)r;
	}
	return (got);
}

/* The whole file (first MK_MAX bytes), NUL-terminated, xmalloc'd; NULL when
   it is not a readable regular file.  Plain open/read/close: nothing in
   libft reads a whole file by name. */
char	*mk_slurp(const char *path, size_t *len)
{
	struct stat	sb;
	int			fd;
	char		*buf;
	size_t		cap;

	if (stat(path, &sb) != 0 || !S_ISREG(sb.st_mode))
		return (NULL);
	fd = open(path, O_RDONLY | O_CLOEXEC);
	if (fd < 0)
		return (NULL);
	cap = (size_t)sb.st_size;
	if (cap > MK_MAX)
		cap = MK_MAX;
	buf = xmalloc(cap + 1);
	if (buf)
	{
		*len = mk_fill_buf(fd, buf, cap);
		buf[*len] = '\0';
	}
	close(fd);
	return (buf);
}

/* One physical line appended to out; 1 when a backslash continues it.  The
   backslash must be unescaped (odd run) and followed by a real newline. */
static int	mk_phys(const char *b, size_t len, size_t *pos, char *out)
{
	size_t	s;
	size_t	e;
	size_t	bs;
	int		nl;

	s = *pos;
	e = s;
	while (e < len && b[e] != '\n')
		e++;
	nl = (e < len);
	*pos = e + (size_t)nl;
	if (e > s && b[e - 1] == '\r')
		e--;
	ft_memcpy(out, b + s, e - s);
	bs = 0;
	while (bs < e - s && b[e - 1 - bs] == '\\')
		bs++;
	out[e - s] = '\0';
	if ((bs & 1) && nl)
		out[e - s - 1] = ' ';
	return ((bs & 1) && nl);
}

/* Join physical lines into the logical one in `out`; returns its length. */
static size_t	mk_logical(const char *b, size_t len, size_t *pos, char *out)
{
	size_t	n;
	int		more;

	n = 0;
	more = 1;
	while (more)
	{
		more = mk_phys(b, len, pos, out + n);
		n += ft_strlen(out + n);
	}
	return (n);
}

/* Walk the buffer line by line.  A logical line is never longer than the
   buffer, so one scratch allocation serves the whole walk. */
void	mk_scan(t_mk_scan *m, const char *buf, size_t len)
{
	char	*line;
	size_t	pos;

	line = xmalloc(len + 1);
	if (!line)
		return ;
	pos = 0;
	while (pos < len)
	{
		mk_logical(buf, len, &pos, line);
		mk_line(m, line);
	}
	xfree(line);
}
