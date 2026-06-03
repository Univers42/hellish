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
#include "version.h"
#include <sys/wait.h>
#include <unistd.h>

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

/* Show every supported way to upgrade. */
static void	print_install_hint(void)
{
	ft_printf("  upgrade with any of:\n");
	ft_printf("    npm i -g %s@latest\n", HELLISH_PKG);
	ft_printf("    docker pull <login>/%s:latest\n", HELLISH_PKG);
	ft_printf("    curl -fsSL https://raw.githubusercontent.com/"
		HELLISH_REPO "/main/install.sh | sh\n");
	ft_printf("    (or just run:  update --now)\n");
}

/* Run the canonical install script to self-update the binary. */
static int	run_installer(void)
{
	char *const	av[] = {"sh", "-c", "curl -fsSL "
		"https://raw.githubusercontent.com/" HELLISH_REPO
		"/main/install.sh | sh", NULL};
	pid_t		pid;
	int			st;

	ft_eprintf("hellish: running installer...\n");
	pid = fork();
	if (pid < 0)
		return (1);
	if (pid == 0)
	{
		execvp(av[0], av);
		_exit(127);
	}
	waitpid(pid, &st, 0);
	return (0);
}

/* `update`: check GitHub for a newer release; `--now` self-updates the binary,
   `--version` just prints the running version. Refreshes the banner's cache. */
int	builtin_update(t_shell *state, t_vec argv)
{
	char	latest[64];
	char	**av;

	(void)state;
	av = (char **)argv.ctx;
	if (argv.len > 1 && !ft_strcmp(av[1], "--version"))
	{
		ft_printf("hellish %s\n", HELLISH_VERSION);
		return (0);
	}
	if (argv.len > 1 && !ft_strcmp(av[1], "--now"))
		return (run_installer());
	ft_eprintf("hellish: checking for updates\xe2\x80\xa6\n");
	if (!fetch_latest_tag(latest, sizeof(latest)))
	{
		ft_eprintf("hellish: could not reach GitHub (offline?)\n");
		return (1);
	}
	hellish_write_cache(latest);
	print_status(latest);
	if (hellish_version_cmp(latest, HELLISH_VERSION) > 0)
		print_install_hint();
	return (0);
}
