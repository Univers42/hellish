/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   prompt_git5.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/17 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/09/17 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "prompt_private.h"

/* The one git scan cache. It used to be a static inside git_dirty_cached;
   vcs_info reads the counts from it too, so it is reachable by name. */
t_dcache	*git_dcache(void)
{
	static t_dcache	c;

	return (&c);
}

/* Whether scans should list untracked files (`-unormal`) or not (`-uno`).
   Off by default: the \g star is about tracked changes, and on a huge tree
   the untracked walk is the expensive part of `git status`. vcs_info turns
   it on for a prompt that asks through HELLISH_GIT_UNTRACKED; a change of
   mode retires the cached answer like a command in the tree does. */
int	*git_untracked_cell(void)
{
	static int	untracked;

	return (&untracked);
}

/* Ahead/behind/stash from the published answer, when that answer is for
   `root`. The scan is keyed on the repository root, so a stale answer for
   a repository the shell has left reads as zeros, never as another repo's
   counts. */
t_gitcounts	git_counts(const char *root)
{
	t_gitcounts	r;
	t_dcache	*c;

	ft_bzero(&r, sizeof(r));
	c = git_dcache();
	if (!root || !c->init || ft_strcmp(c->root, root) != 0)
		return (r);
	r.ahead = c->cur.ahead;
	r.behind = c->cur.behind;
	r.stash = c->cur.stash;
	return (r);
}

/* A write redirection was opened on `path`: the tree may have changed,
** unless the target is a device.
**
** `cmd 2>/dev/null` counted as a tree change like `cmd > file` does, and
** a prompt hook is full of those. Every bare Enter under such a hook
** retired the cached answer and started a new `git status`, which is the
** one thing the scan generation exists to prevent (prompt_git2.c). Nothing
** under /dev/ is part of a working tree. */
void	git_redir_touch(const char *path)
{
	if (path && ft_strncmp(path, "/dev/", 5) == 0)
		return ;
	git_tree_touched();
}

/* Whether the last HEAD the prompt read named a commit rather than a
   branch (prompt_metadata.c). vcs_info reads it right after that read. */
int	*git_detached_cell(void)
{
	static int	detached;

	return (&detached);
}
