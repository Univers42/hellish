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

/* The left column: a greeting, the 42 logo mascot, the hellish identity and the
   current directory. HP_LOGO lines are centred and tinted with logo_color. */
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
	l[i + 3] = "hellish " HELLISH_VERSION " \xc2\xb7 a POSIX shell with bite";
	l[i + 4] = cwd;
	l[i + 5] = NULL;
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
	const char	*l[16];
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
