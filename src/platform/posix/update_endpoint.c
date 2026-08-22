/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   update_endpoint.c                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/20 00:40:00 by marvin            #+#    #+#             */
/*   Updated: 2026/08/20 00:40:00 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "update.h"
#include "version.h"
#include "sys.h"
#include <sys/utsname.h>
#include <unistd.h>
#include <libgen.h>

/* Where release metadata is read from.  GitHub by default, but overridable
   with HELLISH_UPDATE_API so the whole flow can be driven against a local
   server in tests -- issue #20 asks for the updater not to be welded to one
   distribution mechanism, and a test that cannot point it somewhere else
   would have to reach the real internet to prove anything. */
void	update_api_url(char *out, size_t n)
{
	const char	*env;

	env = getenv("HELLISH_UPDATE_API");
	if (env && *env)
		return ((void)ft_strlcpy(out, env, n));
	ft_strlcpy(out, "https://api.github.com/repos/" HELLISH_REPO
		"/releases/latest", n);
}

/* The release asset this machine can actually run.  Returns 0 when no
   binary is published for the running OS/arch, which the caller must treat
   as "cannot self-update" rather than downloading something unusable. */
int	update_asset_name(char *out, size_t n)
{
	struct utsname	u;

	if (uname(&u) != 0)
		return (0);
	if (ft_strcmp(u.sysname, "Linux") != 0)
		return (0);
	if (ft_strcmp(u.machine, "x86_64") != 0
		&& ft_strcmp(u.machine, "aarch64") != 0)
		return (0);
	ft_strlcpy(out, "hellish-linux-", n);
	ft_strlcat(out, u.machine, n);
	return (1);
}

/* Download URL for one release asset.  HELLISH_UPDATE_DL overrides the base
   for the same reason update_api_url is overridable. */
int	update_asset_url(const char *tag, const char *asset, char *out, size_t n)
{
	const char	*base;

	base = getenv("HELLISH_UPDATE_DL");
	if (!base || !*base)
		base = "https://github.com/" HELLISH_REPO "/releases/download";
	if (ft_strncmp(base, "https://", 8) != 0
		&& ft_strncmp(base, "http://127.0.0.1", 16) != 0
		&& ft_strncmp(base, "http://localhost", 16) != 0)
		return (0);
	ft_strlcpy(out, base, n);
	ft_strlcat(out, "/v", n);
	ft_strlcat(out, tag, n);
	ft_strlcat(out, "/", n);
	ft_strlcat(out, asset, n);
	return (1);
}

/* Resolve the running executable.  Everything the installer does is anchored
   on this: which directory to write into, whether that needs privileges, and
   what to replace. */
int	update_exe_path(char *buf, size_t n)
{
	char	*self;

	self = self_exe_path();
	if (!self)
		return (0);
	ft_strlcpy(buf, self, n);
	return (1);
}

/* Can we replace `path` without elevation?  The test is write access to the
   DIRECTORY, not the file: replacing a binary is a rename within its
   directory, so that is the permission that actually matters -- a read-only
   file in a writable directory is replaceable, and a writable file in a
   root-owned directory is not. */
int	update_needs_sudo(const char *path)
{
	char	dir[1024];
	char	*slash;

	ft_strlcpy(dir, path, sizeof(dir));
	slash = ft_strrchr(dir, '/');
	if (!slash)
		return (0);
	if (slash == dir)
		slash[1] = '\0';
	else
		*slash = '\0';
	return (access(dir, W_OK) != 0);
}
