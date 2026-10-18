/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtin_exec_path.c                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/25 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/09/25 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"
#include "cmd_hash.h"
#include <unistd.h>

void	path_cache_sync(t_shell *state);
char	*exe_path_preferred(char **path_dirs, char *exe_name, bool posix);

/* The file as bash's exec names it (full_pathname): a relative path joined
   to the working directory, less one leading "./". execve gets the same
   file either way; this is the name its diagnostic prints -- `exec
   ./noexec` in /tmp/w says "/tmp/w/noexec: Permission denied". Frees path
   when it returns a new string. */
static char	*exec_abspath(t_shell *state, char *path)
{
	char	*cwd;
	char	*rel;
	char	*dir;
	char	*full;

	cwd = (char *)state->cwd.ctx;
	if (!path || *path == '/' || !cwd || !*cwd)
		return (path);
	rel = path;
	if (rel[0] == '.' && rel[1] == '/')
		rel += 2;
	if (cwd[ft_strlen(cwd) - 1] == '/')
		dir = ft_strdup(cwd);
	else
		dir = ft_strjoin(cwd, "/");
	full = NULL;
	if (dir)
		full = ft_strjoin(dir, rel);
	xfree(dir);
	if (!full)
		return (path);
	xfree(path);
	return (full);
}

/* The file `exec NAME` runs, found as the executor finds a command: a
   NAME with a '/' as it stands, else its remembered location, else the
   first match on PATH -- a plain file without execute permission too, as
   bash's exec picks it and then says "Permission denied". NULL when PATH
   has nothing by that name.
     This used to be find_cmd_path, which belongs to a forked child that
   exits next: on a miss it prints its own diagnostic and frees the whole
   shell. exec runs in the shell itself, so `exec nosuch` said "command
   not found", then printed its own message through the freed state (a
   use-after-free under ASan), and an interactive shell could not stay up
   as bash's does. Nothing here prints or frees the shell. */
char	*exec_lookup(t_shell *state, char *name)
{
	char	*hit;

	if (ft_strchr(name, '/'))
		return (exec_abspath(state, ft_strdup(name)));
	path_cache_sync(state);
	hit = cmd_hash_lookup(&state->cmd_cache, name);
	if (hit && access(hit, X_OK) == 0)
		return (exec_abspath(state, ft_strdup(hit)));
	return (exec_abspath(state,
			exe_path_preferred(state->path_dirs, name, false)));
}
