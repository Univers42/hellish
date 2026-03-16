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

static int	handle_perm_denied(t_shell *state, char *cmd_name)
{
	errno = EACCES;
	err_1_errno(state, cmd_name);
	free_all_state(state);
	return (EXE_PERM_DENIED);
}

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
		free(*path_of_exe);
		return (EXE_PERM_DENIED);
	}
	return (0);
}

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

static int	handle_path_lookup(t_shell *state,
				char *cmd_name,
				char **path_of_exe)
{
	char	*path;
	char	**path_dirs;
	int		perm_denied;
	int		ret;
	char	*cached;

	cached = cmd_hash_lookup(&state->cmd_cache, cmd_name);
	if (cached && access(cached, X_OK) == 0)
	{
		*path_of_exe = ft_strdup(cached);
		return (0);
	}
	path = env_expand(state, "PATH");
	path_dirs = NULL;
	if (path)
		path_dirs = ft_split(path, ':');
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
	cmd_hash_insert(&state->cmd_cache, cmd_name, *path_of_exe);
	ret = handle_found_path(state, cmd_name, path_of_exe);
	return (ret);
}

int	find_cmd_path(t_shell *state, char *cmd_name, char **path_of_exe)
{
	if (ft_strchr(cmd_name, '/'))
		return (handle_direct_path(state, cmd_name, path_of_exe));
	return (handle_path_lookup(state, cmd_name, path_of_exe));
}
