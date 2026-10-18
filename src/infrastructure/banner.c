/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   banner.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/03 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/05 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "banner_private.h"
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

/* Where the release's features are explained in full, under the logo. The
   left column is a fixed 43 columns wide, so the link fits there at any
   terminal width; the right column has 32 at 80. Plain text, not an OSC 8
   hyperlink: header_width() skips CSI escapes only, and would count the
   hidden target as visible columns. */
static const char	*guide_link(void)
{
	if (header_use_unicode())
		return ("\033[38;5;209m\xe2\x86\x92\033[0m "
			"\033[4;38;5;81munivers42.github.io/hellish/whats-new\033[0m");
	return ("\033[38;5;209m->\033[0m "
		"\033[4;38;5;81munivers42.github.io/hellish/whats-new\033[0m");
}

/* The left column: a greeting, the 42 logo mascot, the hellish identity, the
   current directory and the link to the release guide. HP_LOGO lines are
   centred and tinted with logo_color. Under an ASCII terminal the identity's
   middle dot is a dash: header_width() cannot decode it there and stops
   counting, which pushed the frame out of line. */
static void	build_left(const char **l)
{
	static char	welcome[128];
	static char	cwd[4096];
	char		*user;
	int			i;

	user = getenv("USER");
	if (!user || !*user)
		user = "friend";
	ft_snprintf(welcome, sizeof(welcome), HP_HEAD "Welcome back, %s!", user);
	build_cwd(cwd, sizeof(cwd));
	l[0] = welcome;
	l[1] = "";
	i = -1;
	while (g_logo[++i])
		l[i + 2] = g_logo[i];
	l[i + 2] = "";
	l[i + 3] = "hellish " HELLISH_VERSION " - a POSIX shell with bite";
	if (header_use_unicode())
		l[i + 3] = "hellish " HELLISH_VERSION
			" \xc2\xb7 a POSIX shell with bite";
	l[i + 4] = cwd;
	l[i + 5] = "";
	l[i + 6] = "\033[38;5;245mthe 3.2 guide, with examples\033[0m";
	l[i + 7] = guide_link();
	l[i + 8] = NULL;
}

/* Print the welcome panel on interactive (tty) startup, when it has
   something new to say -- see banner_should_show(). It used to run on every
   single startup AND wipe the screen and scrollback first, which is what
   made it something to opt out of rather than something to read. Spans the full
   width. The right column reads the cached release check (no network here, so
   startup stays instant) and flags a newer release. Whether it is drawn at
   all is banner_should_show()'s call, knob included -- this function used to
   read HELLISH_NO_BANNER itself as well, which meant the "should it show"
   rule lived in two places and only one of them learned about
   HELLISH_BANNER. */
void	show_welcome(t_shell *state)
{
	const char	*l[20];
	const char	*r[24];
	t_panel		p;

	if (state->metinp != INP_RL || !isatty(STDERR_FILENO))
		return ;
	if (!banner_should_show())
		return ;
	p.title = "hellish " HELLISH_VERSION;
	p.logo_color = "\033[38;5;209m";
	build_left(l);
	build_right(r);
	p.left = l;
	p.right = r;
	render_panel(&p);
	banner_mark_shown();
}
