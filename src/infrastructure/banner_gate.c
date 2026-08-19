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
   extra banner. */
int	banner_should_show(void)
{
	t_upd_state	s;

	if (getenv("HELLISH_ALWAYS_BANNER"))
		return (1);
	update_state_load(&s);
	if (s.header_shown <= 0)
		return (1);
	if (s.header_rev != HELLISH_HEADER_REV)
		return (1);
	if (ft_strcmp(s.header_ver, HELLISH_VERSION) != 0)
		return (1);
	if (update_available(&s) && s.notified <= 0)
		return (1);
	return (!same_day(s.header_shown, (long)time(NULL)));
}

/* Remember that the header was just drawn, with the revision and version
   it was drawn for, so a later change to either brings it back. */
void	banner_mark_shown(void)
{
	t_upd_state	s;

	update_state_load(&s);
	s.header_shown = (long)time(NULL);
	s.header_rev = HELLISH_HEADER_REV;
	ft_strlcpy(s.header_ver, HELLISH_VERSION, sizeof(s.header_ver));
	update_state_save(&s);
}
