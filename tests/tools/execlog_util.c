/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   execlog_util.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/07 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/09/07 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "execlog.h"

/* First line of a `#!` file, past the `#!` and any blanks; "-" otherwise. */
static void	shebang(const char *path, char *out, size_t n)
{
	int		fd;
	ssize_t	got;
	char	buf[256];
	size_t	i;

	out[0] = '-';
	out[1] = '\0';
	fd = open(path, O_RDONLY | O_CLOEXEC);
	if (fd < 0)
		return ;
	got = read(fd, buf, sizeof(buf) - 1);
	close(fd);
	if (got < 3 || buf[0] != '#' || buf[1] != '!')
		return ;
	buf[got] = '\0';
	i = 2;
	while (buf[i] == ' ' || buf[i] == '\t')
		i++;
	snprintf(out, n, "%s", buf + i);
	out[strcspn(out, "\r\n")] = '\0';
}

static void	put_argv(FILE *f, char *const argv[])
{
	int	i;

	i = 0;
	while (argv && argv[i])
	{
		if (i > 0)
			fputc('\x1f', f);
		fputs(argv[i], f);
		i++;
	}
	fputc('\n', f);
}

void	execlog_record(const char *path, char *const argv[])
{
	const char	*log;
	FILE		*f;
	char		sb[256];
	char		self[4096];
	ssize_t		got;

	log = getenv("EXECLOG");
	if (!log || !*log)
		return ;
	f = fopen(log, "a");
	if (!f)
		return ;
	got = readlink("/proc/self/exe", self, sizeof(self) - 1);
	if (got < 0)
		got = 0;
	self[got] = '\0';
	shebang(path, sb, sizeof(sb));
	fprintf(f, "%ld\t%s\t%s\t%s\t", (long)getpid(), self, path, sb);
	put_argv(f, argv);
	fclose(f);
}

/* PATH lookup the way execvp does it, so the log names the file the kernel
** will open (and its shebang can be read). */
const char	*execlog_resolve(const char *file, char *buf, size_t n)
{
	const char	*p;
	const char	*e;

	if (strchr(file, '/'))
		return (file);
	p = getenv("PATH");
	if (!p)
		p = "/usr/local/bin:/usr/bin:/bin";
	while (1)
	{
		e = strchr(p, ':');
		if (!e)
			e = p + strlen(p);
		if (e == p)
			snprintf(buf, n, "./%s", file);
		else
			snprintf(buf, n, "%.*s/%s", (int)(e - p), p, file);
		if (access(buf, X_OK) == 0)
			return (buf);
		if (!*e)
			return (file);
		p = e + 1;
	}
}

char	**execlog_collect(const char *arg0, va_list ap, char ***envp_out)
{
	static char	*argv[1024];
	int			n;

	n = 0;
	argv[n++] = (char *)arg0;
	while (n < 1023)
	{
		argv[n] = va_arg(ap, char *);
		if (!argv[n])
			break ;
		n++;
	}
	argv[n] = NULL;
	if (envp_out)
		*envp_out = va_arg(ap, char **);
	return (argv);
}
