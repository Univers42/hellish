/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   find_cmd_path.c                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 15:10:20 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/22 15:10:24 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "execution_private.h"

int find_cmd_path(t_shell *state, char *cmd_name, char **path_of_exe)
{
	char	*path;
	char	**path_dirs;
	bool	enoent;
	int		ret;
	int		perm_denied;

	/* Handle direct paths (containing '/') separately */
	if (ft_strchr(cmd_name, '/'))
	{
		*path_of_exe = ft_strdup(cmd_name);
		ret = handle_direct_path_error(state, cmd_name, path_of_exe);
		if (ret)
		{
			free_all_state(state);
			return (ret);
		}
		return (0);
	}
	/* PATH lookup for commands without '/' */
	path = env_expand(state, "PATH");
	path_dirs = 0;
	if (path)
		path_dirs = ft_split(path, ':');
	perm_denied = 0;
	*path_of_exe = exe_path(path_dirs, cmd_name, &perm_denied);
	if (path_dirs)
		free_tab(path_dirs);
	if (!*path_of_exe)
	{
		/* Check if we found the file but couldn't execute it */
		if (perm_denied)
		{
			errno = EACCES;
			err_1_errno(state, cmd_name);
			free_all_state(state);
			return (EXE_PERM_DENIED);
		}
		return (cmd_not_found(state, cmd_name));
	}
	enoent = false;
	bool is_dir = check_is_a_dir(*path_of_exe, &enoent);
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
