/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   update_apply.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/19 20:40:00 by marvin            #+#    #+#             */
/*   Updated: 2026/08/19 20:40:00 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "update.h"
#include "version.h"
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

/* Run the freshly downloaded binary and make it tell us its own version.
   This is the step that catches everything a checksum cannot: a build for
   the wrong architecture, a binary that cannot find its libraries, a
   release whose tag and contents disagree. If it will not run HERE, it is
   not going to become this machine's shell. */
static int	validate_binary(const char *path, const char *want)
{
	char		out[128];
	char *const	argv[] = {(char *)path, "-c", "update --version", NULL};
	char		*p;

	if (update_capture(argv, out, sizeof(out)) <= 0)
		return (0);
	p = ft_strchr(out, ' ');
	if (!p)
		return (0);
	p++;
	while (*p && (p[ft_strlen(p) - 1] == '\n' || p[ft_strlen(p) - 1] == ' '))
		p[ft_strlen(p) - 1] = '\0';
	return (ft_strcmp(p, want) == 0);
}

/* Put the verified binary in place. Without elevation this is a rename(2)
   inside the target's own directory, which is atomic: no reader ever sees a
   half-written executable, and the running shell keeps its own open inode,
   so replacing the file underneath it is safe on Linux. With elevation we
   hand the same job to `sudo install`, which is a single auditable command
   rather than a shell we assembled ourselves. */
static int	move_into_place(const char *tmp, const char *target, int sudo)
{
	char *const	argv[] = {"sudo", "install", "-m", "755",
		(char *)tmp, (char *)target, NULL};
	char		out[64];
	int			st;

	if (!sudo)
	{
		if (rename(tmp, target) != 0)
			return (0);
		return (chmod(target, 0755) == 0);
	}
	ft_eprintf("hellish: %s is not writable; running:\n  sudo install -m "
		"755 <download> %s\n", target, target);
	st = (int)update_capture(argv, out, sizeof(out));
	unlink(tmp);
	return (st >= 0 && access(target, X_OK) == 0);
}

/* Build the sibling temp path the download lands on. Same directory as the
   target so the final rename stays inside one filesystem -- across a mount
   point rename(2) fails and there is no atomic replacement at all. */
static int	tmp_path(const char *target, char *out, size_t n)
{
	ft_strlcpy(out, target, n);
	ft_strlcat(out, ".hellish-update", n);
	return (ft_strlen(out) + 1 < n);
}

/* check -> download -> verify -> validate -> atomically replace.
   Returns 0 on success or a step code: 1 no asset for this platform,
   2 download failed, 3 checksum REJECTED, 4 the binary would not run,
   5 the replacement itself failed. Every failure path unlinks the download
   and leaves the installed binary exactly as it was. */
int	update_apply(const char *tag, const char *target, int sudo)
{
	char	asset[64];
	char	url[1024];
	char	tmp[1088];
	int		sha;

	if (!update_asset_name(asset, sizeof(asset))
		|| !update_asset_url(tag, asset, url, sizeof(url))
		|| !tmp_path(target, tmp, sizeof(tmp)))
		return (1);
	if (!update_download(url, tmp, 1024))
		return (unlink(tmp), 2);
	sha = update_verify_sha(tag, asset, tmp);
	if (sha == 0)
		return (unlink(tmp), 3);
	if (sha < 0)
		ft_eprintf("hellish: this release publishes no checksum — "
			"verifying by running the binary instead\n");
	chmod(tmp, 0755);
	if (!validate_binary(tmp, tag))
		return (unlink(tmp), 4);
	if (!move_into_place(tmp, target, sudo))
		return (unlink(tmp), 5);
	return (0);
}
