/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   find_cmd_path.c                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 15:10:20 by dlesieur          #+#    #+#             */
/*   Updated: 2026/03/16 01:53:46 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "execution_private.h"
#include "cmd_hash.h"

int	handle_perm_denied(t_shell *state, char *cmd_name);

/* Post-process a path that passed the initial access(F_OK) check.  A race
   can make the file disappear between access() and stat(), yielding ENOENT
   here; we distinguish that from an EISDIR (tried to exec a directory).
   Both are errors; both free the path string and return an error code. */
static int	handle_found_path(t_shell *state,
				char *cmd_name,
				char **path_of_exe)
{
	bool	is_dir;
	bool	enoent;

	enoent = false;
	is_dir = check_is_a_dir(*path_of_exe, &enoent);
	if (enoent)
		return (no_such_file_or_dir(state, cmd_name, *path_of_exe));
	if (is_dir)
	{
		errno = EISDIR;
		err_1_errno(state, cmd_name);
		free_all_state(state);
		xfree(*path_of_exe);
		return (EXE_PERM_DENIED);
	}
	return (0);
}

/* Handle a command name that contains '/': use it as-is (no PATH search).
   We still validate it through handle_direct_path_error to produce the
   right error message and exit code for missing/non-executable files. */
static int	handle_direct_path(t_shell *state,
				char *cmd_name,
				char **path_of_exe)
{
	int	ret;

	*path_of_exe = ft_strdup(cmd_name);
	ret = handle_direct_path_error(state, cmd_name, path_of_exe);
	if (ret)
	{
		free_all_state(state);
		return (ret);
	}
	return (0);
}

/* Walk PATH directories looking for cmd_name.  We split $PATH on ':' each
   call (no caching of the split) because PATH can change between commands.
   The perm_denied flag from exe_path distinguishes exit code 126 (found
   but not executable) from 127 (not found). */
static int	search_path_dirs(t_shell *state, char *cmd_name,
				char **path_of_exe)
{
	char	**path_dirs;
	int		perm_denied;

	path_dirs = NULL;
	if (env_expand(state, "PATH"))
		path_dirs = ft_split(env_expand(state, "PATH"), ':');
	perm_denied = 0;
	*path_of_exe = exe_path(path_dirs, cmd_name, &perm_denied);
	if (path_dirs)
		free_tab(path_dirs);
	if (!*path_of_exe)
	{
		if (perm_denied)
			return (handle_perm_denied(state, cmd_name));
		return (cmd_not_found(state, cmd_name));
	}
	return (0);
}

/* Command-hash cache (like bash `hash`): check the cache first; if the
   cached path is still executable we skip the PATH walk entirely.  On a
   miss, search PATH, insert the result, then validate the found path.  The
   access(X_OK) guard on the cached entry evicts stale entries (the binary
   was deleted or its permissions changed) so `hash` never causes a
   persistent "command not found" for commands that should exist. */
static int	handle_path_lookup(t_shell *state,
				char *cmd_name,
				char **path_of_exe)
{
	char	*cached;
	int		ret;

	cached = cmd_hash_lookup(&state->cmd_cache, cmd_name);
	if (cached && access(cached, X_OK) == 0)
	{
		*path_of_exe = ft_strdup(cached);
		return (0);
	}
	ret = search_path_dirs(state, cmd_name, path_of_exe);
	if (ret)
		return (ret);
	cmd_hash_insert(&state->cmd_cache, cmd_name, *path_of_exe);
	return (handle_found_path(state, cmd_name, path_of_exe));
}

/* Top-level path resolution: direct path (has '/') or PATH hash + search.
   Returns 0 + *path_of_exe filled on success, or an error code and
   *path_of_exe=NULL on failure.  All error paths have already printed
   the relevant diagnostic before returning. */
int	find_cmd_path(t_shell *state, char *cmd_name, char **path_of_exe)
{
	if (ft_strchr(cmd_name, '/'))
		return (handle_direct_path(state, cmd_name, path_of_exe));
	return (handle_path_lookup(state, cmd_name, path_of_exe));
}
