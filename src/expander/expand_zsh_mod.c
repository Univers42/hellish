/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   expand_zsh_mod.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/30 09:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/30 09:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"
#include "env.h"

/* zsh history-style modifiers on a parameter: ${x:h} ${x:t} ${x:r} ${x:e}
** ${x:a} ${x:A} ${x:l} ${x:u}, and they CHAIN -- `${0:A:h}` is "make it
** absolute, then take the directory", which is how every zsh plugin finds
** its own directory:
**
**     typeset -g __colored_man_pages_dir="${0:A:h}"
**
** Hooked ahead of pf_find_substr, which would otherwise read `x:h` as the
** substring form `${x:offset}` and evaluate `h` as arithmetic -- giving
** offset 0 and handing back the whole value. That is the failure mode worth
** naming: not an error, just the unmodified string, which reads as "the
** modifier did nothing" and sends a plugin looking in the wrong directory.
*/

/* One modifier applied to `v` (owned); returns a fresh string, or NULL for a
   letter this does not implement -- the caller then leaves the whole
   expansion alone rather than silently returning `v` unchanged. */
static char	*zm_apply(t_shell *state, char *v, char m)
{
	if (m == 'h')
		return (zm_head(v));
	if (m == 't')
		return (zm_tail(v));
	if (m == 'r')
		return (zm_root(v));
	if (m == 'e')
		return (zm_ext(v));
	if (m == 'a' || m == 'A')
		return (zm_abs(state, v));
	if (m == 'l')
		return (zf_case(v, 'L'));
	if (m == 'u')
		return (zf_case(v, 'U'));
	if (m == 'q')
		return (zf_quote(v, '\''));
	return (NULL);
}

/* Is the tail of a ${...} body a run of `:x` modifiers?  Every colon must be
   followed by a letter this file knows and then either another colon or the
   end; one unknown letter rejects the whole run, so `${x:offset}` and
   `${x:-word}` are never mistaken for one. */
static bool	zm_is_run(const char *s, int slen, int at)
{
	int	i;

	if (at >= slen || s[at] != ':')
		return (false);
	i = at;
	while (i < slen)
	{
		if (s[i] != ':' || i + 1 >= slen)
			return (false);
		if (!ft_strchr("htreaAluq", s[i + 1]))
			return (false);
		i += 2;
	}
	return (true);
}

/* Apply every modifier in the run, left to right. */
static char	*zm_run(t_shell *state, char *v, const char *s, int slen)
{
	char	*next;
	int		i;

	i = 0;
	while (i + 1 < slen && v)
	{
		next = zm_apply(state, v, s[i + 1]);
		xfree(v);
		v = next;
		i += 2;
	}
	return (v);
}

/* ${name:mods}. Returns NULL when the body is not that shape, so the caller
   falls through to the bash forms untouched. */
char	*zsh_modifier(t_shell *state, const char *s, int slen)
{
	char	*v;
	char	*raw;
	int		nl;

	if (!zsh_mode(state))
		return (NULL);
	nl = pf_scan_name(s, slen);
	if (nl <= 0 || !zm_is_run(s, slen, nl))
		return (NULL);
	raw = env_expand_n(state, (char *)s, nl);
	if (!raw)
		v = ft_strdup("");
	else
		v = ft_strdup(raw);
	return (zm_run(state, v, s + nl, slen - nl));
}
