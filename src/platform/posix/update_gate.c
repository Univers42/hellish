/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   update_gate.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/23 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/23 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "update.h"
#include <unistd.h>
#include <time.h>

/* Split from update_cache.c only because the norm caps a file at 5
   functions. This is the whole of "should we look again, and may we". */

/* How long to wait before asking again, and the two answers are not the
   same question.

   ALREADY PENDING: nothing left to learn. The badge is on the prompt and
   the banner has announced it; a second opinion changes nothing a user can
   see. A day is plenty.

   BELIEVED CURRENT: this is the ONLY state in which a release can exist
   without us knowing, so it is the only one where asking buys anything.
   A flat day here is what produced issue #64 -- checked at noon, 2.7.4
   published at half past, and every session until the next day reported
   "up to date" with total confidence. A quarter of an hour instead.

   The asymmetry is what keeps it cheap: the frequent interval applies only
   while there is genuinely something to find, and stops the moment it is
   found. HELLISH_UPDATE_TTL still overrides both, which is how the older
   tests force a re-check without waiting. */
long	check_interval(const t_upd_state *s)
{
	const char	*ttl;

	ttl = getenv("HELLISH_UPDATE_TTL");
	if (ttl && *ttl)
		return (ft_atoi(ttl));
	if (update_available(s))
		return (86400);
	return (900);
}

/* True when the last ATTEMPT is recent enough to skip a new one.

   Attempts, not successes. run_bg_update_check() writes `checked` only
   after it has actually learned something, so keying the interval off it
   meant a machine that could not reach the release server re-forked a
   check on every single startup, forever -- the one shape of this code
   that really would hammer. `attempted` backs off on failure too, while
   `checked` keeps its meaning of "last time we learned something", which
   is what the banner's "50m ago" reports. A state file from an older
   hellish has no `attempted`, so fall back to `checked` rather than
   treating it as never-attempted and stampeding on first run. */
int	cache_is_fresh(void)
{
	t_upd_state	s;
	long		last;

	if (!update_state_load(&s))
		return (0);
	last = s.attempted;
	if (last <= 0)
		last = s.checked;
	if (last <= 0)
		return (0);
	return ((long)time(NULL) - last <= check_interval(&s));
}

/* Claim the check before forking, in the PARENT.
   Twenty terminals opened at once would otherwise all read the same stale
   record and all fork their own request. Recording the attempt here makes
   the first one win and the other nineteen see a fresh cache. It is one
   small temp-file-and-rename, at most once per interval, and it happens
   off the prompt's path entirely. */
void	claim_attempt(void)
{
	t_upd_state	s;

	update_state_load(&s);
	s.attempted = (long)time(NULL);
	update_state_save(&s);
}
