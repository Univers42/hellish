/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ai_context.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/22 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/07/06 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ai.h"
#include <readline/history.h>
#include <sys/utsname.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <stdlib.h>

/* os (full context only), cwd, and the last command's exit status -- the
   single most predictive signal for what the user does next. The status is
   exported per interactive turn by ai_prompt_prep, so it reaches the forked
   readline child through environ too. */
static void	ctx_head(char *buf, size_t cap, int lite)
{
	struct utsname	u;
	char			cwd[1024];
	char			*st;

	if (!lite && uname(&u) == 0)
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
	st = getenv("HELLISH_LAST_STATUS");
	if (st && *st)
	{
		ft_strlcat(buf, "last exit status: ", cap);
		ft_strlcat(buf, st, cap);
		ft_strlcat(buf, "\n", cap);
	}
}

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

/* Append the last `n` shell commands (oldest first) from readline history. */
static void	ctx_hist(char *buf, size_t cap, int n)
{
	HIST_ENTRY	**h;
	int			i;

	h = history_list();
	if (!h || history_length == 0)
		return ;
	ft_strlcat(buf, "recent commands:\n", cap);
	i = history_length - n;
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

/* Append up to `max` non-hidden entries of the current directory. */
static void	ctx_dir(char *buf, size_t cap, int max)
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
	while (e && n < max)
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

/* Build the shell-context preamble so the model can infer intent. as_cmd
   (inline completion) gets a lite, focused cut -- fewer prompt tokens is
   directly faster on a CPU backend and less noise for a small model; chat
   keeps the rich version. State-free, so it works in the readline child too.
   xfree the result. ponytail: bounded 8 KB buffer, truncated, never grown. */
char	*ai_context_for(int as_cmd)
{
	char	*buf;
	int		cmds;
	int		files;

	buf = xmalloc(8192);
	ft_strlcpy(buf, "[shell context]\n", 8192);
	cmds = 15;
	files = 60;
	if (as_cmd)
	{
		cmds = 5;
		files = 20;
	}
	ctx_head(buf, 8192, as_cmd);
	ctx_git(buf, 8192);
	ctx_hist(buf, 8192, cmds);
	ctx_dir(buf, 8192, files);
	return (buf);
}
