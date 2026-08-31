/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   prompt_ps1e.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/31 02:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/31 02:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "prompt_private.h"
#include "version.h"

/* Longest \D{...} strftime format kept; named because norminette rejects
   the (int)sizeof spelling. */
#define PS1_FMT_CAP 127

/* The tail of bash's escape set: \r \V \l \! \#.
**
** These existed in bash for twenty years and were simply missing here, so
** a legacy PS1 built around them -- `PS1='[\!] \u\$ '` is a classic --
** printed the escape's own spelling into every prompt. Not an error, which
** is exactly why nobody reported it: the shell worked and the prompt
** looked wrong. Found by the compatibility matrix
** (tests/prompt_compat_matrix_test.py), which asserts a known escape's raw
** spelling never survives rendering.
**
** \A is the ONE bash escape deliberately not restored: hellishrc.example
** documents it as the animation glyph, shadowing bash's 24-hour clock, and
** rc files in the wild already use it that way. \t covers the time.
*/

/* \l: the basename of this terminal's device -- "0" for /dev/pts/0, the
   way bash reports it -- or "tty" when stdin is not a terminal at all,
   which is bash's fallback spelling too. */
static void	ps1_tty(t_string *out)
{
	char	*name;
	char	*slash;

	name = ttyname(STDIN_FILENO);
	if (!name)
		return ((void)vec_push_str(out, "tty"));
	slash = ft_strrchr(name, '/');
	if (slash && slash[1])
		vec_push_str(out, slash + 1);
	else
		vec_push_str(out, name);
}

/* \! and \#, both 1-based like bash. \! is the history number the NEXT
   command will get, so it keeps counting across sessions -- the loaded
   file's entries are part of the numbering. \# restarts at 1 per session
   and comes from its own counter (state->cmd_no, bumped once per REPL
   turn): deriving it from history froze the number on consecutive
   duplicates, because history dedupes and bash's counter does not --
   tests/ps1_bash_escapes_test.py runs `true` three times and watches the
   number move. */
static void	ps1_histno(t_shell *state, t_string *out, char c)
{
	char	*s;

	if (c == '#')
		s = ft_itoa(state->cmd_no);
	else if (!state->hist.hist_active)
		return ((void)vec_push_char(out, '0'));
	else
		s = ft_itoa((int)state->hist.hist_cmds.len + 1);
	if (s)
		vec_push_str(out, s);
	xfree(s);
}

/* The second bash-compatible dispatch, tried after ps1_escape_misc and
   before the hellish extensions. False falls through, so an unknown
   escape still ends up printed literally, exactly like bash. */
bool	ps1_escape_bash2(t_shell *state, t_string *out, char c)
{
	if (c == 'r')
		return (vec_push_char(out, '\r'), true);
	if (c == 'V')
		return (vec_push_str(out, HELLISH_VERSION), true);
	if (c == 'l')
		return (ps1_tty(out), true);
	if (c == '!' || c == '#')
		return (ps1_histno(state, out, c), true);
	return (false);
}

/* \D{format}: strftime with the user's own format, like bash. bash treats
   an empty format as the locale's time representation, i.e. %X. The whole
   zsh time family (%T %t %@ %* %D %W %w and %D{...}) is rewritten onto
   this one escape, so both prompt languages share a single clock. */
static void	ps1_strftime(t_string *out, const char *f, int *i)
{
	char		fmt[PS1_FMT_CAP + 1];
	char		buf[256];
	struct tm	tm;
	time_t		now;
	int			j;

	j = *i + 3;
	while (f[j] && f[j] != '}')
	{
		if (j - *i - 3 < PS1_FMT_CAP)
			fmt[j - *i - 3] = f[j];
		j++;
	}
	if (j - *i - 3 > PS1_FMT_CAP)
		fmt[PS1_FMT_CAP] = '\0';
	else
		fmt[j - *i - 3] = '\0';
	*i = j + (f[j] == '}');
	if (!fmt[0])
		ft_strlcpy(fmt, "%X", sizeof(fmt));
	now = time(NULL);
	localtime_r(&now, &tm);
	if (strftime(buf, sizeof(buf), fmt, &tm) > 0)
		vec_push_str(out, buf);
}

/* The escapes that consume MORE than two characters: \nnn octal and
   \D{format}. Dispatched before the fixed `*i += 2` in ps1_escape, so
   they manage the cursor themselves. */
bool	ps1_escape_span(t_string *out, const char *f, int *i)
{
	char	c;

	c = f[*i + 1];
	if (c >= '0' && c <= '7')
		return (ps1_octal(out, f, i), true);
	if (c == 'D' && f[*i + 2] == '{')
		return (ps1_strftime(out, f, i), true);
	return (false);
}
