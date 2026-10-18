/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   rl_suggest5.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/17 12:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/10/17 12:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "rl_private.h"

/* Scanning the history for suggestions, and the state of Alt-N / Alt-P
** cycling. A cycle remembers the history INDICES it has shown (sg_cyc, a
** stack: Alt-N pushes the next older distinct one, Alt-P pops back), so
** going back to a newer alternative costs nothing and a revisit never
** re-scans. */

/* True when `s` holds nothing a terminal would interpret. 0x7f is DEL; the
   pair 0xC2 0x80..0x9F is U+0080..U+009F, which some terminals read as a
   control sequence introducer. */
static bool	sg_clean(const unsigned char *s)
{
	size_t	i;

	i = 0;
	while (s[i])
	{
		if (s[i] < 0x20 || s[i] == 0x7f)
			return (false);
		if (s[i] == 0xc2 && s[i + 1] >= 0x80 && s[i + 1] < 0xa0)
			return (false);
		i++;
	}
	return (true);
}

/* True when an entry with the same full text was already offered in this
   cycle. Distinct means distinct text: history keeps duplicates, and
   stepping through three copies of one command is not a choice. */
static bool	sg_offered(t_shell *state, t_rl *l, const char *text)
{
	char	**a;
	size_t	*v;
	size_t	i;

	a = (char **)state->hist.hist_cmds.ctx;
	v = (size_t *)l->sg_cyc.ctx;
	i = 0;
	while (i < l->sg_cyc.len)
	{
		if (!ft_strcmp(a[v[i]], text))
			return (true);
		i++;
	}
	return (false);
}

/* The index of the next older entry below `from` that starts with the
** line, is longer than it, is clean and was not offered yet; -1 if none.
** Only the newest sg_max entries are looked at (HELLISH_SUGGEST_MAX), so a
** huge history cannot slow a key down.
**
** Cost: O(sg_max * (line + offered)) per call, offered being the entries
** shown so far in this cycle -- a handful in practice. */
ssize_t	rl_suggest_scan(t_shell *state, t_rl *l, size_t from)
{
	char	**a;
	size_t	floor;
	size_t	n;

	a = (char **)state->hist.hist_cmds.ctx;
	n = (size_t)rl_end;
	floor = 0;
	if (state->hist.hist_cmds.len > (size_t)l->sg_max)
		floor = state->hist.hist_cmds.len - (size_t)l->sg_max;
	while (from > floor)
	{
		from--;
		if (a[from] && !ft_strncmp(a[from], rl_line_buffer, n)
			&& ft_strlen(a[from]) > n && sg_clean((unsigned char *)a[from])
			&& !sg_offered(state, l, a[from]))
			return ((ssize_t)from);
	}
	return (-1);
}

/* Forget the cycle (not the entry on screen). */
void	rl_suggest_cyc_clear(t_rl *l)
{
	xfree(l->sg_cyc.ctx);
	l->sg_cyc.ctx = NULL;
	l->sg_cyc.len = 0;
	l->sg_cyc.cap = 0;
	l->sg_cyc_k = 0;
	xfree(l->sg_cyc_line);
	l->sg_cyc_line = NULL;
}

/* Any change to the line ends a cycle, and the shown alternative with it:
** the cached scan result is dropped too, or the fast path of
** rl_suggest_find would keep serving the alternative for the longer line
** instead of the newest match. So does a change in the history's length:
** a bind -x widget can run `history -c` mid-line, and the cycle's indices
** would then point past the end. True when it did reset.
**
** Caveat: only the length is compared, so a widget that deletes one entry
** and adds another keeps a cycle whose indices are in bounds but may name
** different text until the line changes. */
bool	rl_suggest_cyc_sync(t_shell *state, t_rl *l)
{
	if (l->sg_hn == state->hist.hist_cmds.len && (!l->sg_cyc_line
			|| !ft_strcmp(l->sg_cyc_line, rl_line_buffer)))
		return (false);
	rl_suggest_cyc_clear(l);
	l->sg_known = false;
	return (true);
}
