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
#include <stdio.h>
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

/* Release the split PATH array, the cached PATH string, and any directory
   still open.  Called both when the generator is done and at init to reset
   from a prior session -- readline abandons a generator the moment it has
   what it needs, so without closing dh here an unfinished TAB would leak
   its open DIR into the next one. */
void	cmd_gen_cleanup(t_cmd_gen *g)
{
	free_split(g->dirs);
	g->dirs = NULL;
	xfree(g->cache);
	g->cache = NULL;
	if (g->dh)
		closedir(g->dh);
	g->dh = NULL;
}

/* Reset the generator: re-read PATH from the environment (not from our
   internal env vec -- readline completion runs without a t_shell pointer)
   and split on ':'.  Both cursors rewind so builtins are offered again
   before the PATH scan. */
void	cmd_gen_init(t_cmd_gen *g)
{
	cmd_gen_cleanup(g);
	g->bidx = 0;
	g->idx = 0;
	if (getenv("PATH"))
		g->cache = ft_strdup(getenv("PATH"));
	if (g->cache)
		g->dirs = ft_split(g->cache, ':');
}

/* Return the next directory entry whose name starts with `text`, or NULL
   when the directory is exhausted.  The match is libc-allocated because
   readline frees it (see rl_dup in completion.c).  `d` stays open: the
   caller resumes this scan on the next generator call. */
char	*cmd_gen_scan_dir(DIR *d, const char *text, size_t tlen)
{
	struct dirent	*ent;

	while (1)
	{
		ent = readdir(d);
		if (!ent)
			break ;
		if (ft_strncmp(ent->d_name, text, tlen) == 0)
			return (rl_dup(ent->d_name));
	}
	return (NULL);
}

/* Walk PATH, yielding EVERY match in a directory before moving to the
   next.  The handle is held open in the generator state across calls;
   closing it after the first hit -- which is what this used to do, since
   readline gets exactly one match per call -- silently dropped every
   other command in that directory.  An empty TAB then listed about one
   command per PATH entry instead of all of them.  An unopenable
   directory just leaves dh NULL and the loop moves on. */
char	*cmd_gen_dirs(t_cmd_gen *g, size_t tlen, const char *text)
{
	char	*name;

	while (1)
	{
		if (g->dh)
		{
			name = cmd_gen_scan_dir(g->dh, text, tlen);
			if (name)
				return (name);
			closedir(g->dh);
			g->dh = NULL;
		}
		if (!g->dirs || !g->dirs[g->idx])
			break ;
		g->dh = opendir(g->dirs[g->idx]);
		g->idx++;
	}
	return (cmd_gen_cleanup(g), NULL);
}
