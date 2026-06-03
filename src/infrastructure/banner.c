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

/* A dim rounded rule (l...r) spanning the banner width. */
static void	banner_rule(const char *l, const char *r)
{
	int	i;

	ft_eprintf("  \033[38;5;238m%s", l);
	i = -1;
	while (++i < 61)
		ft_eprintf("\xe2\x94\x80");
	ft_eprintf("%s\033[0m\n", r);
}

/* The little horned emblem + the name and version. */
static void	banner_logo(void)
{
	ft_eprintf("\n     \033[38;5;208m\xe2\x96\x9f\xe2\x96\x99 ");
	ft_eprintf("\033[38;5;196m\xe2\x96\x9f\xe2\x96\x99\033[0m");
	ft_eprintf("     \033[1;38;5;214mhellish\033[0m");
	ft_eprintf("   \033[38;5;240mv%s\033[0m\n", HELLISH_VERSION);
	ft_eprintf("     \033[38;5;202m\xe2\x96\x9d\xe2\x96\x88\xe2\x96\x88");
	ft_eprintf("\xe2\x96\x9b\033[0m");
	ft_eprintf("      \033[38;5;245ma fast, POSIX shell ");
	ft_eprintf("\xe2\x80\x94 with horns\033[0m\n\n");
}

/* The two hint rows; the second turns into an update notice when one is due. */
static void	banner_tips(int has_update, const char *latest)
{
	ft_eprintf("              \033[38;5;208m\xe2\x80\xba\033[0m  ");
	ft_eprintf("\033[38;5;252mhelp\033[0m     list builtins\n");
	ft_eprintf("              \033[38;5;208m\xe2\x80\xba\033[0m  ");
	if (has_update)
	{
		ft_eprintf("\033[38;5;220mupdate\033[0m   ");
		ft_eprintf("\033[38;5;220m\xe2\xac\x86 v%s ready\033[0m", latest);
		ft_eprintf(" \xe2\x80\x94 run \033[1mupdate\033[0m\n\n");
	}
	else
	{
		ft_eprintf("\033[38;5;252mupdate\033[0m   ");
		ft_eprintf("check for a newer version\n\n");
	}
}

/* Print the welcome banner once, on interactive (tty) startup. Opt out with
   HELLISH_NO_BANNER. Reads the last background check to flag a new release. */
void	show_welcome(t_shell *state)
{
	char	latest[64];
	int		has_update;

	if (state->metinp != INP_RL || !isatty(STDERR_FILENO))
		return ;
	if (getenv("HELLISH_NO_BANNER"))
		return ;
	has_update = (read_cached_latest(latest, sizeof(latest))
			&& hellish_version_cmp(latest, HELLISH_VERSION) > 0);
	banner_rule("\xe2\x95\xad", "\xe2\x95\xae");
	banner_logo();
	banner_tips(has_update, latest);
	banner_rule("\xe2\x95\xb0", "\xe2\x95\xaf");
}
