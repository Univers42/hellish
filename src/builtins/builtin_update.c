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

#include "builtins_private.h"
#include "update.h"
#include "update_menu.h"
#include "version.h"

/* Report how the running version compares to the latest release tag. */
static void	print_status(const char *latest)
{
	if (hellish_version_cmp(latest, HELLISH_VERSION) <= 0)
	{
		ft_printf("\033[32m\xe2\x9c\x93\033[0m hellish %s", HELLISH_VERSION);
		ft_printf(" is the latest release.\n");
		return ;
	}
	ft_printf("\033[33m\xe2\xac\x86\033[0m hellish %s is available ", latest);
	ft_printf("(you have %s).\n", HELLISH_VERSION);
}

/* Strongly nudge the user to upgrade, tailored to where this copy came from. */
static void	print_update_hint(t_origin o, const char *repo)
{
	char	cmd[1024];

	origin_command(o, repo, cmd, sizeof(cmd));
	ft_printf("  \033[1;38;5;203mYou should update.\033[0m installed via "
		"\033[1m%s\033[0m — upgrade with:\n", origin_label(o));
	ft_printf("    %s\n", cmd);
	ft_printf("  or run \033[1mupdate --now\033[0m to pick a method and "
		"reinstall the binary yourself.\n");
}

/* Live-check GitHub, refresh the banner cache, and report + hint. */
static int	do_check(t_origin origin, const char *repo)
{
	char	latest[64];

	ft_eprintf("hellish: checking for updates\xe2\x80\xa6\n");
	if (!fetch_latest_tag(latest, sizeof(latest)))
	{
		ft_eprintf("hellish: could not reach GitHub (offline?)\n");
		return (1);
	}
	hellish_write_cache(latest);
	print_status(latest);
	if (hellish_version_cmp(latest, HELLISH_VERSION) > 0)
		print_update_hint(origin, repo);
	return (0);
}

/* `update`: compare against GitHub's latest release and, for `--now`, run the
   upgrade that matches how this binary was installed (npm/pnpm/docker/source/
   standalone). `--version` prints the running version. */
int	builtin_update(t_shell *state, t_vec argv)
{
	char		repo[512];
	t_origin	origin;
	char		**av;

	(void)state;
	av = (char **)argv.ctx;
	if (argv.len > 1 && !ft_strcmp(av[1], "--version"))
	{
		ft_printf("hellish %s\n", HELLISH_VERSION);
		return (0);
	}
	origin = detect_origin(repo, sizeof(repo));
	if (argv.len > 1 && !ft_strcmp(av[1], "--now"))
		return (update_interactive(origin, repo));
	return (do_check(origin, repo));
}
