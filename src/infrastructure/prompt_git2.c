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

/* Bumped once per command the REPL actually executes.

   The dirty cache below throttles how often `git status` is RE-RUN while
   nothing is happening, which is right: a prompt redrawn on an idle
   terminal has no reason to rescan. What it must never do is outlive an
   actual change to the working tree -- and the only moment the tree can
   change under this shell is a command running in it.

   Without this, a scan that took a second armed a 30-second TTL and the
   prompt then asserted "dirty" for half a minute after `git checkout`,
   `git commit` or `git stash` had made it clean. The star said one thing
   and `git status` on the line above said the other.

   A counter rather than a flag: it makes "has anything happened since
   this answer was computed?" a comparison, so a scan that completes late
   is still attributed to the generation it was started for. */
int	*git_scan_gen(void)
{
	static int	gen;

	return (&gen);
}
