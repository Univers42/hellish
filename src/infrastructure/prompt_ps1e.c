/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   prompt_ps1e.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/31 12:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/31 12:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "prompt_private.h"
#include "version.h"
#include <time.h>
#include <unistd.h>

/* The bash escapes that used to fall through to literal passthrough --
** #72 phase 3.7.
**
** The fallback in ps1_escape ("unknown escape? emit backslash + char") is
** correct for an escape bash does not know either, and it silently WRONG for
** one bash does: a PS1 copied out of a .bashrc came back with `\!` sitting
** in it as two visible characters. No error, no status, just a prompt that
** reads like the shell is broken.
**
** \A is the one collision left standing. bash's \A is the 24-hour clock;
** hellish's is the animation frame and shipped first, so every existing
** prompt using it would change meaning. Documented in ps1_escape_ext and
** pinned by tests/ps1_bash_escapes_test.py rather than quietly swapped.
*/

/* strftime into the prompt. Shared by \D{}, \T and \@ so the three cannot
   disagree about the locale or the time they were called at. */
static void	ps1_strf(t_string *out, const char *fmt)
{
	char		buf[256];
	time_t		now;
	struct tm	tm;

	now = time(NULL);
	localtime_r(&now, &tm);
	if (strftime(buf, sizeof(buf), fmt, &tm) > 0)
		vec_push_str(out, buf);
}

/* \D{format} -- the only escape that takes an argument, which is why it is
   handled where the format string and the cursor are still in reach rather
   than in the character dispatch. *i points just past the D. An empty
   format is bash's %X, the locale's time; a missing brace is not a \D at
   all and goes back out as it came in. */
void	ps1_dfmt(t_string *out, const char *f, int *i)
{
	char	fmt[128];
	size_t	n;

	if (f[*i] != '{')
	{
		vec_push_str(out, "\\D");
		return ;
	}
	n = 0;
	*i += 1;
	while (f[*i] && f[*i] != '}' && n + 1 < sizeof(fmt))
		fmt[n++] = f[(*i)++];
	fmt[n] = '\0';
	if (f[*i] == '}')
		*i += 1;
	if (n == 0)
		ft_strlcpy(fmt, "%X", sizeof(fmt));
	ps1_strf(out, fmt);
}

/* \! is the history number the next command will get, \# the number of
   this command in this session. They are close enough to be confused and
   are not the same: history persists across sessions, the counter does
   not, so a fresh shell shows \!=214 and \#=1. */
static void	ps1_counter(t_shell *state, t_string *out, char c)
{
	char	*n;

	if (c == '!')
		n = ft_itoa((int)state->hist.hist_cmds.len + 1);
	else
		n = ft_itoa(state->cmd_no);
	if (n)
		vec_push_str(out, n);
	xfree(n);
}

/* \l: the basename of the terminal device, which is what bash prints --
   `pts/3`, not `/dev/pts/3`. Nothing at all when stdin is not a tty,
   because there is no terminal to name. */
static void	ps1_tty(t_string *out)
{
	char	*name;
	char	*slash;

	name = ttyname(STDIN_FILENO);
	if (!name)
		return ;
	slash = ft_strrchr(name, '/');
	if (slash)
		vec_push_str(out, slash + 1);
	else
		vec_push_str(out, name);
}

/* The zero-argument half of the set. \v already prints the version and
   keeps doing so; \V is bash's fuller spelling and here they agree,
   because hellish has one version string and inventing a second would be
   a difference with nothing behind it. */
bool	ps1_escape_bash(t_shell *state, t_string *out, char c)
{
	if (c == 'r')
		return (vec_push_char(out, '\r'), true);
	if (c == 'T')
		return (ps1_strf(out, "%I:%M:%S"), true);
	if (c == '@')
		return (ps1_strf(out, "%I:%M %p"), true);
	if (c == 'V')
		return (vec_push_str(out, HELLISH_VERSION), true);
	if (c == '!' || c == '#')
		return (ps1_counter(state, out, c), true);
	if (c == 'l')
		return (ps1_tty(out), true);
	return (false);
}
