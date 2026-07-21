/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   prompt_git2.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/21 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/07/21 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "prompt_private.h"
#include <sys/stat.h>

/* One stat: does `dir` contain a .git entry (dir, or gitdir file for
   submodules/worktrees — stat is true for both). */
static int	dir_has_git(const char *dir)
{
	char		path[PATH_MAX];
	struct stat	st;

	ft_strlcpy(path, dir, sizeof(path));
	ft_strlcat(path, "/.git", sizeof(path));
	return (stat(path, &st) == 0);
}

/* Walk from loc->cwd toward "/" looking for the repo root. Only runs on a
   cache miss (first prompt, or after cd), so the per-level stats are paid
   once per directory change instead of once per prompt. */
static void	walk_root(t_gitloc *loc)
{
	char	dir[PATH_MAX];
	char	*slash;

	loc->has = 0;
	ft_strlcpy(dir, loc->cwd, sizeof(dir));
	while (1)
	{
		if (dir_has_git(dir))
		{
			ft_strlcpy(loc->root, dir, sizeof(loc->root));
			loc->has = 1;
			return ;
		}
		slash = ft_strrchr(dir, '/');
		if (!slash || (slash == dir && dir[1] == '\0'))
			return ;
		if (slash == dir)
			dir[1] = '\0';
		else
			*slash = '\0';
	}
}

/* Cached repo location for the current directory. On a hit with no repo we
   still stat cwd/.git so a `git init` in place is noticed; a repo appearing
   in a PARENT dir mid-session is only seen after the next cd — the same
   trade fish and starship make. Forked children inherit a copy and re-walk
   on their own cwd, so the cache never leaks stale answers across cds. */
t_gitloc	*repo_locate(void)
{
	static t_gitloc	loc;
	char			now[PATH_MAX];

	if (!getcwd(now, sizeof(now)))
		return (NULL);
	if (loc.init && ft_strcmp(now, loc.cwd) == 0
		&& (loc.has || !dir_has_git(now)))
		return (&loc);
	loc.init = 1;
	ft_strlcpy(loc.cwd, now, sizeof(loc.cwd));
	walk_root(&loc);
	return (&loc);
}
