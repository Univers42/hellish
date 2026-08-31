/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtin_update2.c                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/19 20:40:00 by marvin            #+#    #+#             */
/*   Updated: 2026/08/19 20:40:00 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"
#include "update.h"
#include "update_menu.h"
#include "version.h"
#include <termios.h>
#include <unistd.h>

/* Read a single keypress without waiting for Enter, so [Update] answers
   like a button rather than a form. Falls back to "no" whenever stdin is
   not a terminal: a non-interactive `update` must never block a script
   waiting for a key that will not arrive. */
int	confirm_update(void)
{
	struct termios	saved;
	struct termios	raw;
	char			c;

	if (!isatty(STDIN_FILENO))
		return (0);
	if (tcgetattr(STDIN_FILENO, &saved) != 0)
		return (0);
	raw = saved;
	raw.c_lflag &= ~(ICANON | ECHO);
	raw.c_cc[VMIN] = 1;
	raw.c_cc[VTIME] = 0;
	if (tcsetattr(STDIN_FILENO, TCSANOW, &raw) != 0)
		return (0);
	if (read(STDIN_FILENO, &c, 1) != 1)
		c = 0;
	tcsetattr(STDIN_FILENO, TCSANOW, &saved);
	return (c == 'u' || c == 'U' || c == 'y' || c == 'Y' || c == '\r');
}

/* `update --now`: install without asking again. The version still has to be
   discovered first -- installing "whatever is newest" without knowing what
   that is would leave nothing to verify the download against. */
int	update_now(t_shell *state, t_origin origin, char *repo)
{
	char	latest[64];
	int		got;

	(void)state;
	got = fetch_latest_tag(latest, sizeof(latest));
	if (got == 0)
		return (ft_eprintf("hellish: could not reach the release source "
				"(offline?)\n"), 1);
	if (got < 0)
		return (ft_eprintf("hellish: the release source published no "
				"usable release\n"), 1);
	if (hellish_version_cmp(latest, HELLISH_VERSION) <= 0)
		return (ft_printf("hellish %s is already the latest release.\n",
				HELLISH_VERSION), 0);
	if (update_warn_stale_session(latest))
		return (0);
	return (update_interactive(origin, repo, latest));
}
