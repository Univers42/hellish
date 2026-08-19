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
	if (update_needs_sudo(path))
		return (ORIGIN_BINARY_SYSTEM);
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
	if (o == ORIGIN_BINARY_SYSTEM)
		return ("system binary");
	return ("user binary");
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
		ft_strlcpy(out, "update --now  (downloads and installs the release "
			"binary in place)", n);
}

/* Execute the origin's upgrade.

   A package-managed install delegates to whatever manages it -- that is
   the rule issue #20 lays down, and replacing an npm-owned file behind
   npm's back would only break the next `npm update`. Inside a container we
   cannot help at all, so we print what the host should run.

   A plain release binary is the case we own: update_apply downloads,
   verifies and atomically replaces it. The exit status is the command's
   real status; it used to be a hardcoded 0, so a failed upgrade reported
   success. */
int	run_origin_update(t_origin o, const char *repo, const char *tag)
{
	char		cmd[1024];
	char *const	av[] = {"sh", "-c", cmd, NULL};
	pid_t		pid;
	int			st;

	if (o == ORIGIN_BINARY || o == ORIGIN_BINARY_SYSTEM)
		return (update_selfupdate(o, tag));
	origin_command(o, repo, cmd, sizeof(cmd));
	if (o == ORIGIN_DOCKER)
		return ((void)ft_printf("hellish: run this on the host:\n  %s\n",
				cmd), 0);
	ft_eprintf("hellish: %s\n", cmd);
	pid = fork();
	if (pid < 0)
		return (1);
	if (pid == 0)
		(execvp(av[0], av), _exit(127));
	waitpid(pid, &st, 0);
	if (WIFEXITED(st))
		return (WEXITSTATUS(st));
	return (1);
}
