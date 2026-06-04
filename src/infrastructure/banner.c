/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   banner.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/03 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/03 13:00:18 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "header.h"
#include "update.h"
#include "version.h"
#include <unistd.h>
#include <stdlib.h>

/* The welcome logo: the exact 42 file-header art (the same logo this file is
   topped with). Each line is padded to the same display width so the per-line
   centring keeps the art aligned; it is tinted salmon via logo_color. */
static const char	*g_logo[] = {
	HP_LOGO "        :::      ::::::::",
	HP_LOGO "      :+:      :+:    :+:",
	HP_LOGO "    +:+ +:+         +:+  ",
	HP_LOGO "  +#+  +:+       +#+     ",
	HP_LOGO "+#+#+#+#+#+   +#+        ",
	HP_LOGO "     #+#    #+#          ",
	HP_LOGO "    ###   ########.fr    ",
	NULL
};

/* The left column: a greeting, the dino mascot, the hellish identity and the
   current directory. HP_LOGO lines are centred and tinted with logo_color. */
static void	build_left(const char **l, const char *welcome, const char *cwd)
{
	int	i;

	l[0] = welcome;
	l[1] = "";
	i = -1;
	while (g_logo[++i])
		l[i + 2] = g_logo[i];
	l[i + 2] = "";
	l[i + 3] = "hellish " HELLISH_VERSION " \xc2\xb7 a POSIX shell with bite";
	l[i + 4] = cwd;
	l[i + 5] = NULL;
}

/* The right column: getting-started tips, a divider, then the news block whose
   final highlight line (`news`) is set by the caller. */
static void	build_right(const char **r, const char *news)
{
	r[0] = HP_HEAD "Getting started";
	r[1] = "";
	r[2] = "help      explore commands & options";
	r[3] = "update    fetch the latest release";
	r[4] = HP_RULE;
	r[5] = HP_HEAD "What's new";
	r[6] = "";
	r[7] = news;
	r[8] = "See RELEASE.md for the details";
	r[9] = NULL;
}

/* The news highlight: an update notice when the last background check found a
   newer release, otherwise a friendly default. */
static void	fill_news(char *note, size_t n)
{
	char	latest[64];

	ft_strlcpy(note, "The 42 logo now greets you, in salmon", n);
	if (read_cached_latest(latest, sizeof(latest))
		&& hellish_version_cmp(latest, HELLISH_VERSION) > 0)
		ft_snprintf(note, n, "v%s available - run update", latest);
}

/* The current directory, with a leading $HOME collapsed to "~". */
static void	build_cwd(char *buf, size_t n)
{
	char	cwd[4096];
	char	*home;
	size_t	hl;

	if (!getcwd(cwd, sizeof(cwd)))
	{
		ft_strlcpy(buf, "~", n);
		return ;
	}
	home = getenv("HOME");
	hl = 0;
	if (home)
		hl = ft_strlen(home);
	if (hl && !ft_strncmp(cwd, home, hl) && (!cwd[hl] || cwd[hl] == '/'))
	{
		ft_strlcpy(buf, "~", n);
		ft_strlcat(buf, cwd + hl, n);
		return ;
	}
	ft_strlcpy(buf, cwd, n);
}

/* Print the welcome panel once, on interactive (tty) startup. Spans the full
   width and flags a newer release found by the last background check. Opt out
   with HELLISH_NO_BANNER. */
void	show_welcome(t_shell *state)
{
	const char	*l[16];
	const char	*r[16];
	t_panel		p;
	char		buf[3][256];
	char		*user;

	if (state->metinp != INP_RL || !isatty(STDERR_FILENO))
		return ;
	if (getenv("HELLISH_NO_BANNER"))
		return ;
	play_intro();
	p.title = "hellish " HELLISH_VERSION;
	p.logo_color = "\033[38;5;209m";
	user = getenv("USER");
	if (!user || !*user)
		user = "friend";
	ft_snprintf(buf[0], sizeof(buf[0]), HP_HEAD "Welcome back, %s!", user);
	fill_news(buf[1], sizeof(buf[1]));
	build_cwd(buf[2], sizeof(buf[2]));
	build_left(l, buf[0], buf[2]);
	build_right(r, buf[1]);
	p.left = l;
	p.right = r;
	ft_eprintf("\033[H\033[2J\033[3J");
	render_panel(&p);
}
