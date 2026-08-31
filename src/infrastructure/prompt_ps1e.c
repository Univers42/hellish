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
   file's entries are part of the numbering. \# restarts at 1 per session:
   readmark is exactly "how many entries came from the file", so the
   session count is what sits above it. Derived from history rather than a
   separate counter, so it does not advance on a command history skipped
   (a duplicate, a leading space) -- bash's does; close enough for a
   prompt, and one less piece of state to keep honest. */
static void	ps1_histno(t_shell *state, t_string *out, char c)
{
	size_t	n;
	char	*s;

	n = state->hist.hist_cmds.len;
	if (c == '#' && n >= state->hist.readmark)
		n -= state->hist.readmark;
	else if (c == '#')
		n = 0;
	s = ft_itoa((int)n + 1);
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
