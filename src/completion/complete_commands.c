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

/* Built-in names offered before the PATH scan.  Checked first so the user
   gets `echo` immediately even if /bin/echo appears much later.

   This must stay the set registered in hash_builtins_dispatch.c: the old
   list held 18 of the 52, so `histor<TAB>` or `printf<TAB>` completed to
   nothing at all -- and it listed `env`, which that table's own comment
   says is NOT a builtin here (real env execs its argument, so we let
   /usr/bin/env be found by the PATH scan like any other program).
   completion_posix_test.py reads the dispatch table and fails if the two
   ever drift apart again. */
char	*g_builtins[] = {
	"echo", "export", "cd", "pushd", "popd", "[[", "exit", "pwd", "unset",
	"type", "set", "shift", ":", "break", "continue", "eval", ".",
	"source", "true", "false", "umask", "command", "return", "getopts",
	"exec", "wait", "times", "trap", "readonly", "read", "test", "[",
	"alias", "unalias", "hash", "jobs", "fg", "bg", "fc", "history",
	"let", "local", "kill", "printf", "ulimit", "update", "help",
	"mapfile", "readarray", "declare", "typeset", "shopt", "pretty",
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
	g->cur = NULL;
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

/* Return the next entry of the open directory that is both a prefix match
   for `text` and something this shell could actually execute, or NULL when
   the directory is exhausted.  The match is libc-allocated because readline
   frees it (see rl_dup in completion.c).  g->dh stays open: the caller
   resumes this scan on the next generator call.

   The cmd_entry_runnable() call is the fix for the reported bug.  This used
   to accept every readdir() result whose NAME matched, which is not what a
   PATH element means: POSIX searches it for an executable file, so a 0644
   document, a subdirectory, and the "." and ".." that every directory
   carries are not commands and never were.  Anyone whose PATH held a
   directory that also held data -- ~/bin, ~/.local/bin, ./scripts -- got
   those documents offered by name on the very first TAB. */
char	*cmd_gen_scan_dir(t_cmd_gen *g, const char *text, size_t tlen)
{
	struct dirent	*ent;

	while (1)
	{
		ent = readdir(g->dh);
		if (!ent)
			break ;
		if (ft_strncmp(ent->d_name, text, tlen) == 0
			&& cmd_entry_runnable(g->cur, ent->d_name))
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
			name = cmd_gen_scan_dir(g, text, tlen);
			if (name)
				return (name);
			closedir(g->dh);
			g->dh = NULL;
		}
		if (!g->dirs || !g->dirs[g->idx])
			break ;
		g->cur = g->dirs[g->idx];
		g->dh = opendir(g->cur);
		g->idx++;
	}
	return (cmd_gen_cleanup(g), NULL);
}
