/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtin_update.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/03 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/03 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "brand.h"
#include "builtins_private.h"
#include "update.h"
#include "update_menu.h"
#include "version.h"
#include <time.h>

/* Report how the running version compares to the latest release. Returns 1
   when the DISK is already current and only this process is behind, so the
   caller offers nothing -- downloading in that state is what produces a
   second install in another prefix (see update_stale.c). */
static int	print_status(const char *latest)
{
	if (hellish_version_cmp(latest, HELLISH_VERSION) <= 0)
	{
		ft_printf("\033[32m\xe2\x9c\x93\033[0m polish %s", HELLISH_VERSION);
		ft_printf(" is the latest release.\n");
		return (0);
	}
	ft_printf("\033[33m\xe2\xac\x86\033[0m polish %s is available ", latest);
	ft_printf("(you have %s).\n", HELLISH_VERSION);
	return (update_warn_stale_session(latest));
}

/* Persist what a foreground check just learned, so the prompt notice and the
   next startup agree with it and no second network call is needed. A new
   version clears `notified` so the user is told about it once. */
static void	remember(const char *latest)
{
	t_upd_state	s;

	update_state_load(&s);
	if (ft_strcmp(s.latest, latest) != 0)
		s.notified = 0;
	ft_strlcpy(s.latest, latest, sizeof(s.latest));
	s.checked = (long)time(NULL);
	update_state_save(&s);
}

/* The [Update] / [Later] decision. Explicit by construction: nothing is
   downloaded until the user picks Update, and Later is a first-class answer
   that simply returns. */
static int	offer(t_shell *state, const char *latest, t_origin o, char *repo)
{
	(void)state;
	ft_printf("\n  \033[1;38;5;203m[Update]\033[0m install %s now"
		"      \033[2m[Later]\033[0m keep %s\n", latest, HELLISH_VERSION);
	ft_printf("  installed via \033[1m%s\033[0m\n", origin_label(o));
	ft_printf("  press \033[1mu\033[0m to update, anything else to skip: ");
	if (!confirm_update())
		return (ft_printf("\n"), 0);
	ft_printf("\n");
	return (update_interactive(o, repo, latest));
}

/* Live-check the release source and report. The three fetch outcomes get
   three different messages: a dead endpoint is not the same problem as an
   endpoint with no releases, and calling both "offline" is what hid a
   misconfigured repository URL for this feature's entire life. */
static int	do_check(t_shell *state, t_origin origin, char *repo, int ask)
{
	char	latest[64];
	int		got;

	ft_eprintf("polish: checking for updates\xe2\x80\xa6\n");
	got = fetch_latest_tag(latest, sizeof(latest));
	if (got == 0)
		return (ft_eprintf("polish: could not reach the release source "
				"(offline?)\n"), 1);
	if (got < 0)
		return (ft_eprintf("polish: the release source published no "
				"usable release\n"), 1);
	remember(latest);
	if (print_status(latest))
		return (0);
	if (hellish_version_cmp(latest, HELLISH_VERSION) <= 0)
		return (0);
	if (!ask)
		return (0);
	return (offer(state, latest, origin, repo));
}

/* `update` checks and offers; `--now` goes straight to installing; `--check`
   only reports (for scripts); `--version` prints the running version.

   `--version` is a frozen contract: every installed updater (back to 2.x)
   validates a download by running `<bin> -c "update --version"` and
   comparing everything after the FIRST space to the tag minus its `v`.
   One word, one space, the bare version, nothing else -- "polish 3.4.0".
   tests/brand_compat_test.sh replays that check.

   `update` and `--now` also add the polish command a <= 3.3.0 updater
   could not (update_brand.c): in a user-owned dir before the network, so
   it works offline; the sudo question for a root-owned one only after the
   update's own, which keeps that one the first [y/N] asked.
   `--check` stays read-only. */
int	builtin_update(t_shell *state, t_vec argv)
{
	char		repo[512];
	t_origin	origin;
	char		**av;
	int			st;

	av = (char **)argv.ctx;
	if (argv.len > 1 && !ft_strcmp(av[1], "--version"))
		return (ft_printf(BRAND_NAME " %s\n", HELLISH_VERSION), 0);
	origin = detect_origin(repo, sizeof(repo));
	if (argv.len > 1 && ft_strcmp(av[1], "--now"))
		return (do_check(state, origin, repo, 0));
	update_add_brand(origin, BRAND_TELL);
	if (argv.len > 1)
		st = update_now(state, origin, repo);
	else
		st = do_check(state, origin, repo, 1);
	update_add_brand(origin, BRAND_ASK);
	return (st);
}
