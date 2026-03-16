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

#include "libft.h"
#include <readline/readline.h>
#include <stdlib.h>
#include <dirent.h>
#include <unistd.h>

static char	*g_builtins[] = {
	"echo", "export", "cd", "exit", "pwd", "env", "unset", "type", "set",
	"read", "test", "alias", "unalias", "hash", "jobs", "fg", "bg", "fc",
	NULL
};

static int	g_cmd_idx;
static char	*g_path_dirs_cache;

static void	free_split(char **arr)
{
	int	i;

	if (!arr)
		return ;
	i = 0;
	while (arr[i])
	{
		free(arr[i]);
		i++;
	}
	free(arr);
}

static void	cmd_gen_cleanup(char ***path_dirs)
{
	free_split(*path_dirs);
	*path_dirs = NULL;
	free(g_path_dirs_cache);
	g_path_dirs_cache = NULL;
}

static void	cmd_gen_init(char ***path_dirs, int *dir_idx)
{
	g_cmd_idx = 0;
	cmd_gen_cleanup(path_dirs);
	if (getenv("PATH"))
		g_path_dirs_cache = ft_strdup(getenv("PATH"));
	*path_dirs = g_path_dirs_cache ? ft_split(g_path_dirs_cache, ':') : NULL;
	*dir_idx = 0;
}

static char	*cmd_gen_dirs(char ***path_dirs, int *dir_idx, size_t tlen,
	const char *text)
{
	DIR				*d;
	struct dirent	*ent;
	char			*name;

	while (*path_dirs && (*path_dirs)[*dir_idx])
	{
		d = opendir((*path_dirs)[*dir_idx]);
		(*dir_idx)++;
		if (!d)
			continue ;
		while (1)
		{
			ent = readdir(d);
			if (!ent)
				break ;
			if (ft_strncmp(ent->d_name, text, tlen) == 0)
			{
				name = ft_strdup(ent->d_name);
				closedir(d);
				return (name);
			}
		}
		closedir(d);
	}
	cmd_gen_cleanup(path_dirs);
	return (NULL);
}

static char	*cmd_generator(const char *text, int state_gen)
{
	static char	**path_dirs;
	static int	dir_idx;
	size_t		tlen;
	char		*name;

	tlen = ft_strlen(text);
	if (!state_gen)
		cmd_gen_init(&path_dirs, &dir_idx);
	while (g_builtins[g_cmd_idx])
	{
		name = g_builtins[g_cmd_idx++];
		if (ft_strncmp(name, text, tlen) == 0)
			return (ft_strdup(name));
	}
	return (cmd_gen_dirs(&path_dirs, &dir_idx, tlen, text));
}

char	**complete_commands(const char *text, int start, int end)
{
	(void)start;
	(void)end;
	rl_completion_append_character = ' ';
	return (rl_completion_matches(text, cmd_generator));
}
