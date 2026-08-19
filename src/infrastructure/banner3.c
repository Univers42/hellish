/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   banner3.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/05 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/05 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "banner_private.h"
#include "update.h"
#include "version.h"
#include <sys/stat.h>
#include <time.h>

/* The status-line glyphs, picked once for the terminal (ASCII fallbacks). */
static t_sym	pick_syms(void)
{
	if (header_use_unicode())
		return ((t_sym){"\xe2\x9c\x93", "\xe2\x96\xb2", "\xc2\xb7",
			"\xe2\x80\x94"});
	return ((t_sym){"v", "!", "-", "-"});
}

/* A human "time ago" for the release cache's mtime — how long since the last
   background check ran. "never" when the cache has not been written yet. */
static void	cache_age(char *buf, size_t n)
{
	long	age;

	age = update_last_check_age();
	if (age < 0)
		return ((void)ft_strlcpy(buf, "never", n));
	if (age < 3600)
		ft_snprintf(buf, n, "%dm ago", (int)(age / 60));
	else if (age < 86400)
		ft_snprintf(buf, n, "%dh ago", (int)(age / 3600));
	else
		ft_snprintf(buf, n, "%dd ago", (int)(age / 86400));
}

/* The "all good" footer: version, how this copy was installed, last check. */
static void	ok_status(char *out, size_t n, t_sym s)
{
	char	age[32];
	char	repo[512];

	cache_age(age, sizeof(age));
	ft_snprintf(out, n, HP_OK "%s %s up to date %s via %s %s %s", s.ok,
		HELLISH_VERSION, s.sep, origin_label(detect_origin(repo,
				sizeof(repo))), s.sep, age);
}

/* The status footer: a calm "up to date" line, or a vivid warning that a newer
   release is out (read from the cached background check — no network here). */
void	status_line(char *out, size_t n)
{
	char	latest[64];
	t_sym	s;

	s = pick_syms();
	if (read_cached_latest(latest, sizeof(latest))
		&& hellish_version_cmp(latest, HELLISH_VERSION) > 0)
		ft_snprintf(out, n, HP_WARN "%s %s available %s run update --now",
			s.warn, latest, s.dash);
	else
		ok_status(out, n, s);
}
