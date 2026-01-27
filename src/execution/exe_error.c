/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   exe_error.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 15:09:52 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/27 16:16:39 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "execution_private.h"

bool	check_is_a_dir(char *path, bool *enoent)
{
	struct stat	info;

	*enoent = false;
	if (stat(path, &info) == -1)
	{
		if (errno == ENOENT)
		{
			*enoent = true;
			return (false);
		}
		critical_error_errno_ctx(path);
	}
	return (S_ISDIR(info.st_mode));
}

int	cmd_not_found(t_shell *state, char *cmd_name)
{
	err_2(state, cmd_name, "command not found");
	free_all_state(state);
	return (COMMAND_NOT_FOUND);
}

int	no_such_file_or_dir(t_shell *state,
						char *cmd_name, char *path_of_exe)
{
	errno = ENOENT;
	err_1_errno(state, cmd_name);
	free_all_state(state);
	free(path_of_exe);
	return (COMMAND_NOT_FOUND);
}

static int	errex_code(t_shell *state,
					char *cmd_name,
					char **path_of_exe,
					int ex)
{
	errno = ENOENT;
	err_1_errno(state, cmd_name);
	free(*path_of_exe);
	*path_of_exe = NULL;
	return (ex);
}

/**
 * check if either file, dir, or executable permission error
 */
int	handle_direct_path_error(t_shell *state, char *cmd_name,
									char **path_of_exe)
{
	struct stat	st;

	if (stat(*path_of_exe, &st) == -1)
		return (errex_code(state, cmd_name, path_of_exe, COMMAND_NOT_FOUND));
	if (S_ISDIR(st.st_mode))
		return (errex_code(state, cmd_name, path_of_exe, EXE_PERM_DENIED));
	if (access(*path_of_exe, X_OK) != 0)
	{
		return (errex_code(state, cmd_name, path_of_exe,
				EXE_PERM_DENIED));
	}
	return (0);
}
