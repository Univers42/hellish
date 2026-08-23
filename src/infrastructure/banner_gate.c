/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   banner_gate.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/19 20:40:00 by marvin            #+#    #+#             */
/*   Updated: 2026/08/19 20:40:00 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "update.h"
#include "version.h"
#include <time.h>

/* Same calendar day in local time? Comparing days rather than "less than
   24h ago" is what a user means by "once a day": a shell opened at 09:00
   and again at 09:30 tomorrow should show the header, and one opened twice
   in an afternoon should not. */
static int	same_day(long a, long b)
{
	struct tm	ta;
	struct tm	tb;
	time_t		x;
	time_t		y;

	x = (time_t)a;
	y = (time_t)b;
	if (!localtime_r(&x, &ta) || !localtime_r(&y, &tb))
		return (0);
	return (ta.tm_year == tb.tm_year && ta.tm_yday == tb.tm_yday);
}

/* What the environment says about the header, before the gate gets a vote:
   1 always draw, 0 never draw, -1 no opinion.

   There is one knob, HELLISH_BANNER=0|1, because there used to be two --
   HELLISH_NO_BANNER and HELLISH_ALWAYS_BANNER -- which is two names for the
   two ends of one tri-state, and neither is the name anybody guesses.
   (Issue #56 opens with `BANNER=1 hellish` doing nothing at all.) Both old
   names still work: they are documented, they are in people's rc files, and
   silently dropping them would be a worse bug than the one being fixed.

   Precedence is "off wins", in the order a user would expect to be obeyed:
   an explicit NO_BANNER beats everything, then HELLISH_BANNER, then the
   legacy force. Turning something off is the request you must never get
   wrong -- a stray force in a login file must not be able to override a
   deliberate silence on the command line. */
static int	banner_env_pref(void)
{
	const char	*v;

	if (getenv("HELLISH_NO_BANNER"))
		return (0);
	v = getenv("HELLISH_BANNER");
	if (v && *v)
	{
		if (!ft_strcmp(v, "0") || !ft_strcmp(v, "no")
			|| !ft_strcmp(v, "off"))
			return (0);
		return (1);
	}
	if (getenv("HELLISH_ALWAYS_BANNER"))
		return (1);
	return (-1);
}

/* Should the welcome header be drawn?

   Shell initialisation and header display are deliberately NOT the same
   event (issue #21): the shell always comes up, the header is decoration
   with news in it. It earns a redraw when it has something new to say --
   a new day, a header revision with different content, a hellish that has
   just been upgraded, or an update the user has not been told about --
   and stays quiet otherwise, which is the common case of opening the
   twentieth terminal of the afternoon.

   A missing or unreadable state file means "show it": failing towards
   showing keeps the shell honest about news, and the only cost is one
   extra banner.

   The update clause is the one that was broken (issue #56). It used to ask
   `s.notified <= 0` -- but `notified` belongs to a DIFFERENT channel, the
   prompt's one-shot between-commands notice, and whichever of the two
   fired first silenced the other. The prompt always won: the session that
   discovers a release draws its banner before the background check has
   written anything, and by the next session `notified` was set and the
   banner was gated off for good. So the "X available - run update --now"
   status line, which renders perfectly well, was unreachable in the normal
   flow and users only ever learned of a release by typing `update`.

   The banner now tracks what IT announced, in its own field. Two channels,
   two records, neither able to mute the other. */
int	banner_should_show(void)
{
	t_upd_state	s;
	int			pref;

	pref = banner_env_pref();
	if (pref >= 0)
		return (pref);
	update_state_load(&s);
	if (s.header_shown <= 0)
		return (1);
	if (s.header_rev != HELLISH_HEADER_REV)
		return (1);
	if (ft_strcmp(s.header_ver, HELLISH_VERSION) != 0)
		return (1);
	if (update_available(&s) && ft_strcmp(s.announced, s.latest) != 0)
		return (1);
	return (!same_day(s.header_shown, (long)time(NULL)));
}

/* Remember that the header was just drawn, with the revision and version
   it was drawn for, so a later change to either brings it back -- and which
   pending release it just announced, so it announces each one once and then
   leaves the standing reminder to the prompt badge. Cleared when nothing is
   pending, so the field can never claim an announcement that did not
   happen. */
void	banner_mark_shown(void)
{
	t_upd_state	s;

	update_state_load(&s);
	s.header_shown = (long)time(NULL);
	s.header_rev = HELLISH_HEADER_REV;
	ft_strlcpy(s.header_ver, HELLISH_VERSION, sizeof(s.header_ver));
	if (update_available(&s))
		ft_strlcpy(s.announced, s.latest, sizeof(s.announced));
	else
		s.announced[0] = '\0';
	update_state_save(&s);
}
