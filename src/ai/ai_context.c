/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ai_context.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/22 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/22 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ai.h"
#include <readline/history.h>
#include <sys/utsname.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>

/* Append "git branch: <name>\n" by reading .git/HEAD directly (no fork). */
static void	ctx_git(char *buf, size_t cap)
{
	char	head[256];
	int		fd;
	ssize_t	r;
	char	*ref;

	fd = open(".git/HEAD", O_RDONLY);
	if (fd < 0)
		return ;
	r = read(fd, head, sizeof(head) - 1);
	close(fd);
	if (r <= 0)
		return ;
	head[r] = '\0';
	ref = ft_strnstr(head, "refs/heads/", (size_t)r);
	if (!ref)
		return ;
	ft_strlcat(buf, "git branch: ", cap);
	ft_strlcat(buf, ref + 11, cap);
}

/* Append the last ~15 shell commands (oldest first) from readline history. */
static void	ctx_hist(char *buf, size_t cap)
{
	HIST_ENTRY	**h;
	int			i;

	h = history_list();
	if (!h || history_length == 0)
		return ;
	ft_strlcat(buf, "recent commands:\n", cap);
	i = history_length - 15;
	if (i < 0)
		i = 0;
	while (i < history_length)
	{
		if (h[i] && h[i]->line)
		{
			ft_strlcat(buf, "  ", cap);
			ft_strlcat(buf, h[i]->line, cap);
			ft_strlcat(buf, "\n", cap);
		}
		i++;
	}
}

/* Append up to 60 non-hidden entries of the current directory. */
static void	ctx_dir(char *buf, size_t cap)
{
	DIR				*d;
	struct dirent	*e;
	int				n;

	d = opendir(".");
	if (!d)
		return ;
	ft_strlcat(buf, "files here: ", cap);
	n = 0;
	e = readdir(d);
	while (e && n < 60)
	{
		if (e->d_name[0] != '.')
		{
			ft_strlcat(buf, e->d_name, cap);
			ft_strlcat(buf, "  ", cap);
			n++;
		}
		e = readdir(d);
	}
	closedir(d);
	ft_strlcat(buf, "\n", cap);
}

/* Build a shell-context preamble so the model can infer intent: OS, cwd, git
   branch, recent commands, current directory. State-free, so it works in the
   readline child too. xfree the result. ponytail: bounded 8 KB buffer; context
   is truncated, never grown -- cheap and predictable. */
char	*ai_context(void)
{
	char			*buf;
	char			cwd[1024];
	struct utsname	u;
	size_t			cap;

	cap = 8192;
	buf = xmalloc(cap);
	ft_strlcpy(buf, "[shell context]\n", cap);
	if (uname(&u) == 0)
	{
		ft_strlcat(buf, "os: ", cap);
		ft_strlcat(buf, u.sysname, cap);
		ft_strlcat(buf, "\n", cap);
	}
	if (getcwd(cwd, sizeof(cwd)))
	{
		ft_strlcat(buf, "cwd: ", cap);
		ft_strlcat(buf, cwd, cap);
		ft_strlcat(buf, "\n", cap);
	}
	ctx_git(buf, cap);
	ctx_hist(buf, cap);
	ctx_dir(buf, cap);
	return (buf);
}
