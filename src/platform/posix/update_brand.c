/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   update_brand.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/20 10:00:00 by marvin            #+#    #+#             */
/*   Updated: 2026/11/20 10:00:00 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "update.h"
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>

/* The layout every <= 3.3.0 updater leaves behind after installing 3.4:
   it replaces the file it knows, .../hellish, and has never heard of
   polish, so the new name never reaches PATH. Only the release binary
   origins qualify -- npm, docker and source builds own their command
   names. Fills `link` with the missing sibling; 1 when it is missing.
   An existing polish, of any kind, is never ours to touch. */
static int	brand_link(t_origin o, char *link, size_t n)
{
	char		exe[1024];
	char		*slash;
	struct stat	st;

	if (o != ORIGIN_BINARY && o != ORIGIN_BINARY_SYSTEM)
		return (0);
	if (!update_exe_path(exe, sizeof(exe)))
		return (0);
	slash = ft_strrchr(exe, '/');
	if (!slash || ft_strcmp(slash + 1, "hellish") != 0)
		return (0);
	slash[1] = '\0';
	ft_strlcpy(link, exe, n);
	ft_strlcat(link, "polish", n);
	return (lstat(link, &st) != 0 && errno == ENOENT);
}

/* Root-owned directory: same rule as confirm_elevation -- say the exact
   command, then run it only on y/Y. The link is relative, so it follows
   the binary rather than pinning today's absolute path. */
static int	brand_sudo(char *link)
{
	char	buf[8];
	ssize_t	r;
	char	*av[6];

	ft_eprintf("polish: hellish is now called polish; %s is missing.\n",
		link);
	ft_eprintf("  polish would run: sudo ln -s hellish %s\n", link);
	ft_eprintf("  proceed? [y/N] ");
	r = read(STDIN_FILENO, buf, sizeof(buf) - 1);
	if (r <= 0 || (buf[0] != 'y' && buf[0] != 'Y'))
		return (ft_eprintf("\n"), 1);
	av[0] = "sudo";
	av[1] = "ln";
	av[2] = "-s";
	av[3] = "hellish";
	av[4] = link;
	av[5] = NULL;
	return (update_run_visible(av) != 0);
}

/* 1 when this install still lacks the polish command -- the banner's
   cue to say "run update". */
int	update_missing_brand(void)
{
	char	repo[512];
	char	link[1024];

	return (brand_link(detect_origin(repo, sizeof(repo)), link,
			sizeof(link)));
}

/* Give a legacy hellish install its polish command. A user-owned
   directory gets the symlink straight away, as the installer would have
   made it; BRAND_QUIET (the banner) does that silently, the other modes
   say so. Only BRAND_ASK touches a root-owned one: asked about when stdin
   is a terminal, else, or when declined, the command to run is printed.
   `update` asks only once its own work is done, so its elevation prompt
   is never preceded by a second, unrelated [y/N].
   Returns 0 when there is (now) a polish command or nothing to do. */
int	update_add_brand(t_origin o, int mode)
{
	char	link[1024];

	if (!brand_link(o, link, sizeof(link)))
		return (0);
	if (!update_needs_sudo(link) && symlink("hellish", link) == 0)
	{
		if (mode != BRAND_QUIET)
			ft_printf("\033[32m\xe2\x9c\x93\033[0m added the polish command:"
				" %s \xe2\x86\x92 hellish\n", link);
		return (0);
	}
	if (mode != BRAND_ASK)
		return (1);
	if (isatty(STDIN_FILENO) && brand_sudo(link) == 0
		&& !update_missing_brand())
		return (ft_printf("\033[32m\xe2\x9c\x93\033[0m added the polish "
				"command: %s \xe2\x86\x92 hellish\n", link), 0);
	ft_eprintf("polish: to add the polish command, run:\n"
		"  sudo ln -s hellish %s\n", link);
	return (1);
}
