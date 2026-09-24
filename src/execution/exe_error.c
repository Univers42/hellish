/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   exe_error.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 15:09:52 by dlesieur          #+#    #+#             */
/*   Updated: 2026/03/16 01:14:49 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "execution_private.h"

/* Stat `path` and report whether it is a directory.  The enoent out-param
   lets callers distinguish "does not exist" from "is a directory" without
   a second stat call.  critical_error_errno_ctx is called for unexpected
   stat failures (EPERM etc.) because those indicate a system problem, not
   a user error. */
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

/* Emit "command not found" for cmd_name and free all shell state (we are
   in a child process at this point), returning 127 (POSIX exit code). */
int	cmd_not_found(t_shell *state, char *cmd_name)
{
	err_2(state, cmd_name, "command not found");
	free_all_state(state);
	return (COMMAND_NOT_FOUND);
}

/* path_of_exe existed but stat'd as ENOENT (race condition: file was
   deleted between access() and stat()).  We set errno=ENOENT explicitly
   before calling err_1_errno so the error message is consistent even if
   some intervening call clobbered it. */
int	no_such_file_or_dir(t_shell *state,
						char *cmd_name, char *path_of_exe)
{
	errno = ENOENT;
	err_1_errno(state, cmd_name);
	free_all_state(state);
	xfree(path_of_exe);
	return (COMMAND_NOT_FOUND);
}

/* Report a direct path's failure with the errno the caller left -- the one
   that says WHY -- and let it decide the status as bash does: 127 when the
   file is not there, 126 for everything else. It used to force ENOENT, so
   a file that exists but is not executable, a directory, `./file/x` and a
   symlink loop all read "No such file or directory" (#133: `./life` was
   there), and the last two also came back 127 instead of 126. */
static int	errex_code(t_shell *state, char *cmd_name, char **path_of_exe)
{
	int	ex;

	ex = EXE_PERM_DENIED;
	if (errno == ENOENT)
		ex = COMMAND_NOT_FOUND;
	err_1_errno(state, cmd_name);
	xfree(*path_of_exe);
	*path_of_exe = NULL;
	return (ex);
}

/* Validate a command given as a direct path (contains '/'): stat's own
   errno, EISDIR, or access's EACCES, each reported for what it is.  Shell
   script extensions (.sh/.hell/.hellish) only need to be readable, not
   executable (the shell will interpret them directly), so we skip the
   X_OK check for those.  Everything else requires X_OK (POSIX). */
int	handle_direct_path_error(t_shell *state, char *cmd_name,
									char **path_of_exe)
{
	struct stat	st;
	size_t		len;

	if (stat(*path_of_exe, &st) == -1)
		return (errex_code(state, cmd_name, path_of_exe));
	if (S_ISDIR(st.st_mode))
		return (errno = EISDIR, errex_code(state, cmd_name, path_of_exe));
	len = ft_strlen(*path_of_exe);
	if ((len >= 3 && ft_strcmp(*path_of_exe + len - 3, ".sh") == 0)
		|| (len >= 5 && ft_strcmp(*path_of_exe + len - 5, ".hell") == 0)
		|| (len >= 8 && ft_strcmp(*path_of_exe + len - 8, ".hellish") == 0))
		return (0);
	if (access(*path_of_exe, X_OK) != 0)
		return (errex_code(state, cmd_name, path_of_exe));
	return (0);
}
