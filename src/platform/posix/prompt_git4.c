/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   prompt_git4.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/02 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/09/02 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "prompt_private.h"
#include <unistd.h>

/* Classify `git status --porcelain -uno` output into the GIT_* bits.
**
** Each line is "XY path": X is the index column, Y the work tree column,
** a space meaning "unchanged there". `??` and `!!` (untracked, ignored)
** count in neither, which is also what zsh's vcs_info does by default --
** its %u never lights up for a new file until a user hook says so, and
** -uno keeps git from scanning for them in the first place.
**
** Rename lines ("R  old -> new") classify like any other: only the two
** leading columns are read, then the line is skipped to its newline.
*/
int	git_status_bits(const char *buf, ssize_t n)
{
	ssize_t	i;
	int		bits;

	bits = 0;
	i = 0;
	while (i + 1 < n)
	{
		if (!ft_strchr(" ?!", buf[i]))
			bits |= GIT_STAGED;
		if (!ft_strchr(" ?!", buf[i + 1]))
			bits |= GIT_UNSTAGED;
		while (i < n && buf[i] != '\n')
			i++;
		i++;
	}
	if (bits)
		bits |= GIT_DIRTY;
	return (bits);
}

/* Read what the scanner has written, up to `cap` bytes. -1 when the very
   first read would block -- the scan is still running and there is nothing
   to classify yet. git flushes the whole listing at exit, so the first
   successful read normally holds all of it; the loop mops up a split write
   and stops at EOF, EAGAIN or a full buffer. */
ssize_t	git_drain(int fd, char *buf, ssize_t cap)
{
	ssize_t	n;
	ssize_t	got;

	n = read(fd, buf, cap);
	if (n < 0)
		return (-1);
	got = 0;
	while (n > 0 && got + n < cap)
	{
		got += n;
		n = read(fd, buf + got, cap - got);
	}
	return (got);
}

/* The repository's root, from the same cwd-keyed cache the branch comes
   from, so it costs nothing at the prompt. NULL outside a repository. */
const char	*git_repo_root(void)
{
	t_gitloc	*loc;

	loc = repo_locate();
	if (!loc || !loc->has)
		return (NULL);
	return (loc->root);
}
