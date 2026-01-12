/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   exe_path.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 23:33:21 by marvin            #+#    #+#             */
/*   Updated: 2026/01/09 23:33:21 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "shell.h"
#include "error.h"
#include <stdbool.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
# include "sh_error.h"
# include "env.h"


/* helper prototypes (ensure visible despite include-order */
void	critical_error_errno_context(const char *msg);
char	*env_expand(t_shell *state, char *key);
void	free_tab(char **tab);

// Forward-declare error helpers to avoid implicit-declaration due to include-order
void	err_1_errno(t_shell *state, char *p1);
void	err_2(t_shell *state, char *p1, char *p2);

char	*exe_path(char **path_dirs, char *exe_name)
{
	t_string	temp;
	int			i;

	if (ft_strchr(exe_name, '/'))
		return (ft_strdup(exe_name));
	if (ft_strlen(exe_name) == 0)
		return (0);
	vec_init(&temp);
	i = -1;
	while (path_dirs && path_dirs[++i])
	{
		vec_clear(&temp);
		vec_push_str(&temp, path_dirs[i]);
		char ch = '/';
		vec_push(&temp, &ch);
		vec_push_str(&temp, exe_name);
		if (access((char *)temp.ctx, F_OK) == 0)
			break ;
		else if (path_dirs[i + 1] == 0) {
			free(temp.ctx);
			return NULL;
		}
	}
	if (temp.len > 0)
		return (char *)temp.ctx;
	free(temp.ctx);
	return NULL;
}

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
		critical_error_errno_context(path);
	}
	return (S_ISDIR(info.st_mode));
}

static int	cmd_not_found(t_shell *state, char *cmd_name)
{
	err_2(state, cmd_name, "command not found");
	free_all_state(state);
	return (COMMAND_NOT_FOUND);
}

static int	no_such_file_or_dir(t_shell *state,
				char *cmd_name, char *path_of_exe)
{
	errno = ENOENT;
	err_1_errno(state, cmd_name);
	free_all_state(state);
	free(path_of_exe);
	return (COMMAND_NOT_FOUND);
}

int	find_cmd_path(t_shell *state, char *cmd_name, char **path_of_exe)
{
	char	*path;
	char	**path_dirs;
	bool	enoent;

	path = env_expand(state, "PATH");
	path_dirs = 0;
	if (path)
		path_dirs = ft_split(path, ':');
	*path_of_exe = exe_path(path_dirs, cmd_name);
	if (path_dirs)
		free_tab(path_dirs);
	if (!*path_of_exe)
		return (cmd_not_found(state, cmd_name));
	if (!check_is_a_dir(*path_of_exe, &enoent))
	{
		errno = EISDIR;
		err_1_errno(state, cmd_name);
		free_all_state(state);
		free(*path_of_exe);
		return (EXE_PERM_DENIED);
	}
	else if (enoent)
		return (no_such_file_or_dir(state, cmd_name, *path_of_exe));
	return (0);
}
