/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   update_stale.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/30 20:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/30 20:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "update.h"
#include "sys.h"
#include "version.h"

/* "You have 2.3.2" -- said by a shell whose own /usr/bin/hellish is 2.7.6.
**
** HELLISH_VERSION is compiled in, so `update` compares the release against
** the version of the PROCESS, and a session that was already open when
** `make my_shell` ran is still executing the old image. Everything it says
** about itself is true and every bit of it reads as a bug:
**
**     $ hellish --version          -> 2.7.6   (a NEW process, from PATH)
**     $ update
**     hellish 2.7.6 is available (you have 2.3.2).
**
** Reported on issue #76 as "we did make my_shell and have the good version,
** but update still says the old one". Nothing was wrong with the install --
** the shell being asked was simply not the shell that got installed.
**
** Worse, accepting that offer used to make it real: the old build resolved
** its own origin as a standalone binary and re-ran install.sh, which honours
** its own PREFIX and drops a SECOND copy in /usr/local/bin -- ahead of
** /usr/bin on the default Debian/Ubuntu PATH. One `make my_shell` plus one
** stale-session `update` is all it takes to end up with two hellishes, the
** newer one shadowed by the one you just installed over it.
**
** So when the running image has been replaced, ask the binary that is
** actually on our path what IT is, and answer about that instead.
*/

/* The version the binary now sitting at our own path reports, or 0 if it
   cannot be asked. Runs it rather than trusting the filesystem: the whole
   point is that the file changed underneath us, so its contents are the
   only authority on what it now is. */
int	installed_version(char *out, size_t n)
{
	char		path[1024];
	char		raw[128];
	char *const	argv[] = {path, "-c", "update --version", NULL};
	char		*p;

	if (!update_exe_path(path, sizeof(path)))
		return (0);
	if (update_capture(argv, raw, sizeof(raw)) <= 0)
		return (0);
	p = ft_strchr(raw, ' ');
	if (!p)
		return (0);
	p++;
	while (*p && (p[ft_strlen(p) - 1] == '\n' || p[ft_strlen(p) - 1] == ' '))
		p[ft_strlen(p) - 1] = '\0';
	if (!*p)
		return (0);
	ft_strlcpy(out, p, n);
	return (1);
}

/* Say that this session is stale, and whether the disk is already current.
   Returns 1 when there is nothing left to download -- the caller must then
   NOT offer an update, because performing one is how the duplicate install
   above happens. */
int	update_warn_stale_session(const char *latest)
{
	char	disk[64];
	char	path[1024];

	if (!self_exe_replaced() || !installed_version(disk, sizeof(disk)))
		return (0);
	if (!update_exe_path(path, sizeof(path)))
		return (0);
	ft_eprintf("  \033[1;33m!\033[0m this session is running hellish %s, "
		"but %s\n    was replaced while it was open and now reports "
		"\033[1m%s\033[0m.\n", HELLISH_VERSION, path, disk);
	if (hellish_version_cmp(latest, disk) > 0)
		return (0);
	ft_eprintf("    Nothing to download — just restart the shell:"
		"\n        exec %s\n", path);
	return (1);
}
