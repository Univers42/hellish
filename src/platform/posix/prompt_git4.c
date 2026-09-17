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
#include <errno.h>

/* A `# ...` header of `git status --porcelain=v2 --branch --show-stash`:
   `# branch.ab +A -B` is ahead/behind the upstream (absent without one),
   `# stash N` the stash count (absent at zero, and on a git older than
   2.35, which prints no stash header -- zero is the right answer there
   as far as a prompt can tell). */
static void	gs_header(t_gitstat *acc, const char *l)
{
	const char	*minus;

	if (ft_strncmp(l, "# branch.ab +", 13) == 0)
	{
		acc->ahead = ft_atoi(l + 13);
		minus = ft_strchr(l + 13, '-');
		if (minus)
			acc->behind = ft_atoi(minus + 1);
	}
	else if (ft_strncmp(l, "# stash ", 8) == 0)
		acc->stash = ft_atoi(l + 8);
}

/* Classify one porcelain v2 line into the GIT_* bits.
**
** `1 XY ...` (changed) and `2 XY ...` (renamed or copied): X is the index
** column and Y the work tree one, `.` meaning "unchanged there" -- what
** zsh's vcs_info renders as %c and %u (issue #112). `u XY ...` is an
** unmerged path, which v1 reported as both columns changed; it keeps
** doing that, so the star and %c/%u do not move, and adds GIT_UNMERGED.
** `? path` is untracked, and only appears when the scan asked for it.
** GIT_DIRTY -- the \g star -- is derived at publish time from the two
** tracked bits, so an untracked file still never lights it. */
static void	gs_line(t_gitstat *acc, const char *l)
{
	if (l[0] == '#')
		gs_header(acc, l);
	else if ((l[0] == '1' || l[0] == '2') && l[1] == ' ' && l[2] && l[3])
	{
		if (l[2] != '.')
			acc->bits |= GIT_STAGED;
		if (l[3] != '.')
			acc->bits |= GIT_UNSTAGED;
	}
	else if (l[0] == 'u' && l[1] == ' ')
		acc->bits |= GIT_STAGED | GIT_UNSTAGED | GIT_UNMERGED;
	else if (l[0] == '?' && l[1] == ' ')
		acc->bits |= GIT_UNTRACKED;
}

/* Feed bytes of scanner output through the line assembler. A read can
   stop anywhere, so the current line is carried in the cache; only its
   first GS_LINE - 1 bytes are kept, which is all a classification reads. */
void	gs_feed(t_dcache *c, const char *buf, ssize_t n)
{
	ssize_t	i;

	i = -1;
	while (++i < n)
	{
		if (buf[i] == '\n')
		{
			c->line[c->llen] = '\0';
			gs_line(&c->acc, c->line);
			c->llen = 0;
		}
		else if (c->llen < GS_LINE - 1)
			c->line[c->llen++] = buf[i];
	}
}

/* Read what the scanner has written so far. 1 when the scan is over: EOF,
   a read error, or nothing left to learn -- every bit the answer can hold
   is already set, and the headers come first, so the rest of a long
   listing would change nothing and is not waited for. 0 when more may
   come (EAGAIN), or when this call has read its share: a huge listing is
   drained over several renders rather than stalling one. */
int	gs_drain(t_dcache *c)
{
	char	buf[4096];
	ssize_t	n;
	int		want;
	int		rounds;

	want = GIT_STAGED | GIT_UNSTAGED | GIT_UNMERGED;
	if (c->untracked)
		want |= GIT_UNTRACKED;
	rounds = 0;
	while (rounds++ < 64)
	{
		n = read(c->fd, buf, sizeof(buf));
		if (n == 0 || (n < 0 && errno != EAGAIN && errno != EINTR))
			return (1);
		if (n < 0)
			return (0);
		gs_feed(c, buf, n);
		if ((c->acc.bits & want) == want)
			return (1);
	}
	return (0);
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
