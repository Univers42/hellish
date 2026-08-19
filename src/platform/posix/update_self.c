/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   update_self.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/19 20:40:00 by marvin            #+#    #+#             */
/*   Updated: 2026/08/19 20:40:00 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "update.h"
#include "version.h"
#include <unistd.h>

/* Human wording for each way update_apply can refuse. Every one of them
   leaves the installed binary untouched, so the message says what stopped
   rather than warning about damage that did not happen. */
static const char	*apply_error(int code)
{
	if (code == 1)
		return ("no release binary is published for this OS/architecture");
	if (code == 2)
		return ("the download failed");
	if (code == 3)
		return ("the checksum did NOT match — download rejected");
	if (code == 4)
		return ("the downloaded binary would not run here");
	return ("the binary could not be put in place");
}

/* Ask before doing anything that needs root. Issue #20 is explicit: never
   silently run a privileged command, and never hand root an unverified
   download. By the time we ask, the binary is already downloaded, checksum
   -verified and proven to run -- the only thing left for sudo to do is the
   move. Answering anything but y/Y walks away with nothing changed. */
static int	confirm_elevation(const char *target)
{
	char	buf[8];
	ssize_t	r;

	ft_eprintf("hellish: %s is owned by another user.\n", target);
	ft_eprintf("  hellish would run: sudo install -m 755 <verified "
		"download> %s\n", target);
	ft_eprintf("  proceed? [y/N] ");
	r = read(STDIN_FILENO, buf, sizeof(buf) - 1);
	if (r <= 0)
		return (ft_eprintf("\n"), 0);
	buf[r] = '\0';
	return (buf[0] == 'y' || buf[0] == 'Y');
}

/* Replace this machine's hellish with release `tag`.

   The running process is never overwritten in the sense that matters: the
   replacement is a rename over the path, and this process keeps executing
   from its own open inode. So the update cannot corrupt the session the
   user is typing into -- it takes effect the next time a hellish starts,
   which is why we report that instead of restarting anything. */
int	update_selfupdate(t_origin o, const char *tag)
{
	char	target[1024];
	int		sudo;
	int		rc;

	if (!update_exe_path(target, sizeof(target)))
		return (ft_eprintf("hellish: cannot locate my own binary\n"), 1);
	sudo = (o == ORIGIN_BINARY_SYSTEM || update_needs_sudo(target));
	if (sudo && !isatty(STDIN_FILENO))
		return (ft_eprintf("hellish: %s needs elevation; run `update --now`"
				" from a terminal\n", target), 1);
	if (sudo && !confirm_elevation(target))
		return (ft_eprintf("hellish: update cancelled\n"), 1);
	ft_eprintf("hellish: installing %s into %s…\n", tag, target);
	rc = update_apply(tag, target, sudo);
	if (rc != 0)
		return (ft_eprintf("hellish: update failed — %s\n"
				"  the installed binary is unchanged.\n",
				apply_error(rc)), 1);
	ft_printf("\033[32m✓\033[0m updated %s → %s\n", HELLISH_VERSION, tag);
	ft_printf("  restart hellish (or open a new shell) to run it.\n");
	return (0);
}
