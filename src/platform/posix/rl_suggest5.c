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

/* Collecting suggestion candidates from the history, and the state of
** Alt-N / Alt-P cycling. A cycle is the ranking (rl_suggest_rank) made
** once, as history INDICES in sg_rank, and a position in it: Alt-N and
** Alt-P only move the position, so nothing is ever scanned again. */

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

/* Fill the scratch with every candidate of the window of the newest
** sg_max entries: starts with the line, is longer than it, is clean.
** Returns how many. The scratch and its capacity are kept for the next
** call (a key never allocates once it has grown), and the texts point into
** the history, so they are good until the history changes.
**
** Cost: O(sg_max * line). */
size_t	rl_suggest_cands(t_shell *state, t_rl *l)
{
	char		**a;
	size_t		i;
	size_t		n;
	t_sg_ent	e;

	a = (char **)state->hist.hist_cmds.ctx;
	n = (size_t)rl_end;
	l->sg_scr.elem_size = sizeof(t_sg_ent);
	l->sg_scr.len = 0;
	i = hist_window_floor(state->hist.hist_cmds.len, l->sg_max);
	while (i < state->hist.hist_cmds.len)
	{
		if (a[i] && !ft_strncmp(a[i], rl_line_buffer, n)
			&& ft_strlen(a[i]) > n && sg_clean((unsigned char *)a[i]))
		{
			e = (t_sg_ent){a[i], i, 0};
			vec_push(&l->sg_scr, &e);
		}
		i++;
	}
	return (l->sg_scr.len);
}

/* Forget the cycle (not the entry on screen). The ranking buffers keep
   their capacity: they are freed with the read (rl_suggest_end). */
void	rl_suggest_cyc_clear(t_rl *l)
{
	l->sg_rank.len = 0;
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
