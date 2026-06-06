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

/* Command completion helpers used by the readline generator in
   complete_commands2.c.  The generator is stateful (readline calls it
   repeatedly with increasing state_gen until it returns NULL), so we use
   file-scope statics for the PATH scan.  cmd_gen_cleanup/init reset them
   between completion sessions so stale directory handles can't leak. */

#include "completion_private.h"
#include "libft.h"
#include <readline/readline.h>
#include <stdlib.h>
#include <dirent.h>
#include <unistd.h>

/* Built-in names offered before PATH scan.  Checked first so the user
   gets `echo` immediately even if /bin/echo appears much later. */
char	*g_builtins[] = {
	"echo", "export", "cd", "exit", "pwd", "env", "unset", "type", "set",
	"read", "test", "alias", "unalias", "hash", "jobs", "fg", "bg", "fc",
	NULL
};

int		g_cmd_idx;
char	*g_path_dirs_cache;

/* Free a NULL-terminated array of strings (result of ft_split). */
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

/* Release the split PATH array and the cached PATH string.  Called both
   when the generator is done and at init to reset from a prior session. */
void	cmd_gen_cleanup(char ***path_dirs)
{
	free_split(*path_dirs);
	*path_dirs = NULL;
	xfree(g_path_dirs_cache);
	g_path_dirs_cache = NULL;
}

/* Reset the generator: re-read PATH from the environment (not from our
   internal env vec -- readline completion runs without a t_shell pointer)
   and split on ':'.  g_cmd_idx rewinds to 0 so builtins are offered
   again before the PATH scan. */
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

/* Return the next directory entry whose name starts with `text`, or NULL
   when the directory is exhausted.  The caller opens and closes `d`. */
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

/* Iterate PATH directories one at a time, advancing dir_idx on each
   call.  Opens the directory, scans for a match, then closes it and
   returns.  If no match in a given dir, tries the next.  When all dirs
   are exhausted it cleans up and returns NULL to signal "done". */
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
