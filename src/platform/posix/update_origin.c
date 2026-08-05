/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   update_origin.c                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/03 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/03 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "update.h"
#include "version.h"
#include <sys/wait.h>
#include <unistd.h>

/* Resolve the path of the running executable. 1 on success. */
static int	exe_path(char *buf, size_t n)
{
	ssize_t	r;

	r = readlink("/proc/self/exe", buf, n - 1);
	if (r <= 0)
		return (0);
	buf[r] = '\0';
	return (1);
}

/* Work out how this hellish was installed, from its own location:
   a pnpm/npm global, a source checkout (…/build/bin/hellish), a container
   (/.dockerenv), or a plain standalone binary. */
t_origin	detect_origin(char *repo, size_t n)
{
	char	path[1024];
	char	*build;

	repo[0] = '\0';
	if (!exe_path(path, sizeof(path)))
		return (ORIGIN_BINARY);
	if (ft_strstr(path, "/.pnpm/"))
		return (ORIGIN_PNPM);
	if (ft_strstr(path, "/node_modules/"))
		return (ORIGIN_NPM);
	build = (char *)ft_strstr(path, "/build/bin/hellish");
	if (build)
	{
		*build = '\0';
		ft_strlcpy(repo, path, n);
		return (ORIGIN_SOURCE);
	}
	if (access("/.dockerenv", F_OK) == 0)
		return (ORIGIN_DOCKER);
	return (ORIGIN_BINARY);
}

/* A short human label for the origin. */
const char	*origin_label(t_origin o)
{
	if (o == ORIGIN_NPM)
		return ("npm");
	if (o == ORIGIN_PNPM)
		return ("pnpm");
	if (o == ORIGIN_DOCKER)
		return ("docker");
	if (o == ORIGIN_SOURCE)
		return ("source checkout");
	return ("standalone binary");
}

/* Build the upgrade command for this origin into out. */
void	origin_command(t_origin o, const char *repo, char *out, size_t n)
{
	if (o == ORIGIN_NPM)
		ft_strlcpy(out, "npm install -g " HELLISH_PKG "@latest", n);
	else if (o == ORIGIN_PNPM)
		ft_strlcpy(out, "pnpm add -g " HELLISH_PKG "@latest", n);
	else if (o == ORIGIN_DOCKER)
		ft_strlcpy(out, "docker pull dlesieur/" HELLISH_PKG ":latest", n);
	else if (o == ORIGIN_SOURCE)
	{
		ft_strlcpy(out, "cd '", n);
		ft_strlcat(out, repo, n);
		ft_strlcat(out, "' && git pull --ff-only && make OPT=1 all", n);
	}
	else
		ft_strlcpy(out, "curl -fsSL https://raw.githubusercontent.com/"
			HELLISH_REPO "/main/install.sh | sh", n);
}

/* Execute the origin's upgrade. Inside a container we can only advise the
   host to re-pull, so we print rather than exec. */
int	run_origin_update(t_origin o, const char *repo)
{
	char		cmd[1024];
	char *const	av[] = {"sh", "-c", cmd, NULL};
	pid_t		pid;
	int			st;

	origin_command(o, repo, cmd, sizeof(cmd));
	if (o == ORIGIN_DOCKER)
	{
		ft_printf("hellish: run this on the host:\n  %s\n", cmd);
		return (0);
	}
	ft_eprintf("hellish: %s\n", cmd);
	pid = fork();
	if (pid < 0)
		return (1);
	if (pid == 0)
	{
		execvp(av[0], av);
		_exit(127);
	}
	waitpid(pid, &st, 0);
	return (0);
}
