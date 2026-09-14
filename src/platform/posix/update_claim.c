/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   update_claim.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/14 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/09/14 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "update.h"
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>

/* Who gets to run the background update check, when several shells start at
   the same moment.

   Recording the attempt before forking was meant to settle that, and it does
   not: cache_is_fresh() reads the record and claim_attempt() writes it, and
   between those two calls every other shell has read the same stale record.
   Six terminals opened together made six requests in CI -- the shape the
   record exists to prevent. Read-then-write is not a claim; it is six
   processes agreeing that nobody has claimed anything yet.

   So the claim is a directory. mkdir(2) either creates it or fails with
   EEXIST, in one step, for every process at once, on a local filesystem and
   on the NFS home a 42 machine gives you -- which O_CREAT | O_EXCL cannot
   promise on NFSv3. One shell creates it and goes on to fetch; the others
   see it exists and drop the check entirely. The winner's child removes it
   when the fetch is over.

   A claim left behind by a killed child would otherwise stop every future
   check, so one older than CLAIM_STALE is reclaimed. The window only has to
   outlive a fetch: `attempted` is written under the claim, so the ordinary
   interval -- a quarter of an hour, or a day -- is what actually keeps the
   next check away. This is just the tie-break. */
#define CLAIM_STALE 60

/* True when the claim is old enough that whoever took it is gone. A claim
   that cannot be stat'd at all has just been released by its owner: say
   stale, and let the caller's mkdir decide. */
static int	claim_is_stale(const char *path)
{
	struct stat	st;

	if (stat(path, &st) != 0)
		return (1);
	return ((long)time(NULL) - (long)st.st_mtime > CLAIM_STALE);
}

/* Take the claim, or report that someone else holds it. `path` comes back
   filled in either way, so the winner can release it later. */
static int	claim_take(char *path, size_t n)
{
	if (!update_cache_mkdir() || !update_cache_file("check.lock", path, n))
		return (0);
	if (mkdir(path, 0755) == 0)
		return (1);
	if (!claim_is_stale(path))
		return (0);
	rmdir(path);
	return (mkdir(path, 0755) == 0);
}

/* Claim the check before forking, in the PARENT, and record the attempt.

   Attempts, not successes: run_bg_update_check() writes `checked` only once
   it has actually learned something, so a machine that cannot reach the
   release server backs off here instead of re-forking a check at every
   single prompt. Returns 0 when another shell owns this round, or when
   there is no cache to write to at all -- a check whose result cannot be
   remembered would run again at the next prompt, forever. */
int	claim_attempt(void)
{
	char		path[512];
	t_upd_state	s;

	if (!claim_take(path, sizeof(path)))
		return (0);
	update_state_load(&s);
	s.attempted = (long)time(NULL);
	update_state_save(&s);
	return (1);
}

/* Give the claim back. Called by the detached child once the fetch is done,
   so the next stale interval is free to check again without waiting out
   CLAIM_STALE. */
void	release_attempt(void)
{
	char	path[512];

	if (update_cache_file("check.lock", path, sizeof(path)))
		rmdir(path);
}
