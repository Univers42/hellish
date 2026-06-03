/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   banner.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/03 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/03 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "update.h"
#include "version.h"
#include <unistd.h>
#include <sys/ioctl.h>

/* The terminal width, so the header can span the whole line. */
static int	term_width(void)
{
	struct winsize	ws;

	if (ioctl(STDERR_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0)
		return (ws.ws_col);
	return (80);
}

/* A full-width rounded rule from corner l to corner r. */
static void	banner_rule(int cols, const char *l, const char *r)
{
	int	i;

	ft_eprintf("\033[38;5;238m%s", l);
	i = 2;
	while (i < cols)
	{
		ft_eprintf("\xe2\x94\x80");
		i++;
	}
	ft_eprintf("%s\033[0m\n", r);
}

/* The mascot's horns + the name, version and one-line description. */
static void	banner_line1(void)
{
	ft_eprintf("\n   \033[38;5;208m\xe2\x97\xa2\xe2\x97\xa3 ");
	ft_eprintf("\xe2\x97\xa2\xe2\x97\xa3\033[0m    ");
	ft_eprintf("\033[1;38;5;214mhellish %s\033[0m", HELLISH_VERSION);
	ft_eprintf("  \033[38;5;245m\xc2\xb7  ");
	ft_eprintf("a fast, POSIX-compliant shell, with horns\033[0m\n");
}

/* The mascot's face + the quick links, and an update notice when one is due. */
static void	banner_line2(int has_update, const char *latest)
{
	ft_eprintf("   \033[38;5;196m(\033[38;5;208m");
	ft_eprintf("\xe2\x80\xa2\xcf\x89\xe2\x80\xa2");
	ft_eprintf("\033[38;5;196m)\033[0m    ");
	ft_eprintf("\033[38;5;240m\xe2\x80\xba\033[0m ");
	ft_eprintf("what's new + how to use: \033[4mRELEASE.md\033[0m   ");
	ft_eprintf("\033[38;5;240m\xe2\x80\xba\033[0m ");
	ft_eprintf("\033[1mhelp\033[0m \xc2\xb7 \033[1mupdate\033[0m\n");
	if (!has_update)
		return ;
	ft_eprintf("           \033[38;5;220m\xe2\xac\x86 v%s", latest);
	ft_eprintf(" available \xe2\x80\x94 run \033[1mupdate\033[0m\n");
}

/* Print the welcome header once, on interactive (tty) startup. Spans the full
   width; flags a newer release found by the last background check. Opt out
   with HELLISH_NO_BANNER. */
void	show_welcome(t_shell *state)
{
	char	latest[64];
	int		has_update;
	int		cols;

	if (state->metinp != INP_RL || !isatty(STDERR_FILENO))
		return ;
	if (getenv("HELLISH_NO_BANNER"))
		return ;
	cols = term_width();
	has_update = (read_cached_latest(latest, sizeof(latest))
			&& hellish_version_cmp(latest, HELLISH_VERSION) > 0);
	banner_rule(cols, "\xe2\x95\xad", "\xe2\x95\xae");
	banner_line1();
	banner_line2(has_update, latest);
	banner_rule(cols, "\xe2\x95\xb0", "\xe2\x95\xaf");
}
