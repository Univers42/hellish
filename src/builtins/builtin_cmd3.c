/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtin_cmd3.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/17 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/09/17 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"
#include "cmd_hash.h"

/* Where `command -v NAME` finds a bare NAME on PATH, or NULL.
**
** Prompt frameworks ask this before every prompt (`command -v git
** >/dev/null`), and it used to split PATH and stat its way down every
** directory each time. While PATH is the string the shell's caches were
** built from, the answer comes from them instead: the remembered location
** first -- bash's `command -v` reports the hashed path too -- then the
** already-split PATH. A PATH that changed since, and was not used to run
** anything yet, is split and walked as before; the hash is not trusted
** across a PATH change (the executor flushes it on its next use). */
char	*command_v_lookup(t_shell *state, const char *name)
{
	char	*path;
	char	*hit;
	char	**dirs;

	path = env_expand(state, "PATH");
	if (!path)
		return (NULL);
	if (state->path_dirs && state->path_dirs_src
		&& ft_strcmp(path, state->path_dirs_src) == 0)
	{
		hit = cmd_hash_lookup(&state->cmd_cache, name);
		if (hit && access(hit, X_OK) == 0)
			return (ft_strdup(hit));
		return (exe_path_preferred(state->path_dirs, (char *)name,
				state->opt_posix));
	}
	dirs = ft_split(path, ':');
	if (!dirs)
		return (NULL);
	path = exe_path_preferred(dirs, (char *)name, state->opt_posix);
	free_tab(dirs);
	return (path);
}
