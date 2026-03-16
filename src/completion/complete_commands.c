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

static char	*cmd_generator(const char *text, int state_gen)
{
	static char	**path_dirs;
	static int	dir_idx;
	size_t		tlen;
	char		*name;

	tlen = ft_strlen(text);
	if (!state_gen)
	{
		g_cmd_idx = 0;
		if (g_path_dirs_cache)
			free(g_path_dirs_cache);
		g_path_dirs_cache = NULL;
		if (getenv("PATH"))
			g_path_dirs_cache = ft_strdup(getenv("PATH"));
		if (path_dirs)
			free(path_dirs);
		path_dirs = g_path_dirs_cache ? ft_split(g_path_dirs_cache, ':') : NULL;
		dir_idx = 0;
	}
	while (g_builtins[g_cmd_idx])
	{
		name = g_builtins[g_cmd_idx++];
		if (ft_strncmp(name, text, tlen) == 0)
			return (ft_strdup(name));
	}
	while (path_dirs && path_dirs[dir_idx])
	{
		DIR				*d;
		struct dirent	*ent;

		d = opendir(path_dirs[dir_idx]);
		dir_idx++;
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
	return (NULL);
}

char	**complete_commands(const char *text, int start, int end)
{
	(void)start;
	(void)end;
	rl_completion_append_character = ' ';
	return (rl_completion_matches(text, cmd_generator));
}
