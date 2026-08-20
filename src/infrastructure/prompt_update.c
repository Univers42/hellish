/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   prompt_update.c                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/20 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/20 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "prompt_private.h"
#include "update.h"
#include "version.h"

#define C_RST2 "\033[0m"

/* Is an update waiting, and which one?  Returns the version string, or NULL.

   This exists because the one-shot notice was too easy to miss. The REPL
   announces a newly discovered version once, between commands, and then
   sets `notified` so it never says it again -- deliberately, because a
   banner on every prompt is how users learn to stop reading banners. But
   "once, ever" means a user who was scrolled away, or who opened the
   terminal that printed it and closed it, never learns there is an update
   at all. The background check only runs daily, so nothing brings it back.

   A badge is the other half of that design: the loud notice stays a
   one-shot, and this quiet marker persists in the prompt for as long as
   the update is actually pending. It costs one line of the prompt row and
   disappears by itself once the new binary is in place.

   The state file is re-read at most every UPD_TAG_TTL seconds rather than
   on every render: the prompt is on the hot path (see HELLISH_PROMPT_BENCH)
   and this would otherwise be an open+read+parse per keystroke-to-prompt
   cycle. Five seconds is far below human notice and far above the cost.

   A NULL state is the prompt benchmark, which renders without a shell. */
const char	*prompt_update_tag(t_shell *state)
{
	t_upd_state	s;
	long		now;

	if (!state || state->metinp != INP_RL
		|| getenv("HELLISH_NO_UPDATE_CHECK"))
		return (NULL);
	now = (long)time(NULL);
	if (state->upd_seen && now - state->upd_seen < UPD_TAG_TTL)
		return (upd_tag_or_null(state));
	state->upd_seen = now;
	state->upd_tag[0] = '\0';
	if (update_state_load(&s) && update_available(&s))
		ft_strlcpy(state->upd_tag, s.latest, sizeof(state->upd_tag));
	return (upd_tag_or_null(state));
}

/* The cached tag, or NULL when the field is empty. Kept separate so the
   "empty means none" rule is stated once instead of at each return. */
const char	*upd_tag_or_null(t_shell *state)
{
	if (!state || !state->upd_tag[0])
		return (NULL);
	return (state->upd_tag);
}

/* The \U prompt escape: " ⬆<version>" when an update is pending, nothing
   otherwise. Self-spacing like \g, \S, \p and \J, so a PS1 can carry it
   unconditionally and it stays invisible until it has something to say. */
void	ps1_update(t_shell *state, t_string *out)
{
	const char	*tag;

	tag = prompt_update_tag(state);
	if (!tag)
		return ;
	vec_push_str(out, " \xe2\xac\x86");
	vec_push_str(out, tag);
}

/* "⬆X.Y.Z" while a newer release is waiting. The loud one-shot notice is
   emitted once between commands and then silenced forever; this is the
   quiet half that persists, so a user who was scrolled away when it
   printed still finds out. Dropped on a narrow row like the other
   accessories -- the box alignment outranks the badge. */
void	render_update_badge(t_string *ret, t_prompt *p)
{
	if (!p->upd || p->cols - p->vis_w <= 30)
		return ;
	vec_push_str(ret, " ");
	vec_push_ansi(ret, pal(PAL_DUR));
	vec_push_str(ret, "\xe2\xac\x86");
	vec_push_str(ret, p->upd);
	vec_push_ansi(ret, C_RST2);
	p->vis_w += measure_width(p->upd) + 2;
}
