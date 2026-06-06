/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   complete_commands.c                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/03/16 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/03/16 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "completion_private.h"
#include "libft.h"
#include <readline/readline.h>
#include <stdlib.h>
#include <dirent.h>
#include <unistd.h>

char	*g_builtins[] = {
	"echo", "export", "cd", "exit", "pwd", "env", "unset", "type", "set",
	"read", "test", "alias", "unalias", "hash", "jobs", "fg", "bg", "fc",
	NULL
};

int		g_cmd_idx;
char	*g_path_dirs_cache;

void	free_split(char **arr)
{
	int	i;

	if (!arr)
		return ;
	i = 0;
	while (arr[i])
	{
		xfree(arr[i]);
		i++;
	}
	xfree(arr);
}

void	cmd_gen_cleanup(char ***path_dirs)
{
	free_split(*path_dirs);
	*path_dirs = NULL;
	xfree(g_path_dirs_cache);
	g_path_dirs_cache = NULL;
}

void	cmd_gen_init(char ***path_dirs, int *dir_idx)
{
	g_cmd_idx = 0;
	cmd_gen_cleanup(path_dirs);
	if (getenv("PATH"))
		g_path_dirs_cache = ft_strdup(getenv("PATH"));
	if (g_path_dirs_cache)
		*path_dirs = ft_split(g_path_dirs_cache, ':');
	else
		*path_dirs = NULL;
	*dir_idx = 0;
}

char	*cmd_gen_scan_dir(DIR *d, const char *text, size_t tlen)
{
	struct dirent	*ent;

	while (1)
	{
		ent = readdir(d);
		if (!ent)
			break ;
		if (ft_strncmp(ent->d_name, text, tlen) == 0)
			return (ft_strdup(ent->d_name));
	}
	return (NULL);
}

char	*cmd_gen_dirs(char ***path_dirs, int *dir_idx, size_t tlen,
	const char *text)
{
	DIR		*d;
	char	*name;

	while (*path_dirs && (*path_dirs)[*dir_idx])
	{
		d = opendir((*path_dirs)[*dir_idx]);
		(*dir_idx)++;
		if (!d)
			continue ;
		name = cmd_gen_scan_dir(d, text, tlen);
		closedir(d);
		if (name)
			return (name);
	}
	cmd_gen_cleanup(path_dirs);
	return (NULL);
}
