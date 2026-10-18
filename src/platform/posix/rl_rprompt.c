/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   rl_rprompt.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/16 15:30:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/09/16 15:30:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "rl_private.h"
#include "zle.h"

/* The right prompt's painter -- the editor half of prompt_rprompt.c.
**
** Installed as rl_redisplay_function for the duration of one readline()
** call. readline redraws the line whenever it pleases (every keystroke,
** ↑/↓ recall, ^L, a resize) and a redraw may end in a clear-to-end-of-
** line that wipes whatever sat to the right of the text. So the clock is
** repainted AFTER every redisplay, from here, and never as part of the
** prompt readline measures: save cursor, jump to the column, print,
** restore. The cursor comes back exactly where readline left it, and
** readline's own column bookkeeping never learns the clock exists --
** which is the point, since teaching it was the bug (prompt_rprompt.c).
**
** DECSC/DECRC (ESC 7 / ESC 8), not CSI s/u: xterm reads ESC[s as DECSLRM
** once left/right margins are enabled, and redraw_mascot already settled
** on the DEC pair for the same kind of frame. The frame goes out in ONE
** write(), for the reason split_prompt spells out: a keystroke echoed
** between two writes lands inside the escape.
**
** Auto-hide, as zsh does: once the typed line would reach the clock the
** clock is erased, and it stays hidden until the line fits again. When
** the cursor may be off the prompt's row -- a wrapped line, a multi-line
** recall, incremental search with its own prompt -- nothing is touched:
** a jump-to-column on the wrong row would clear the user's own text.
**
** Every frame clears from the end of the text, never from the clock's
** column. Two field bugs came from the latter:
**
**   remnants  readline shortens a line with DCH (ESC[nP), which pulls
**             everything right of the cursor n columns LEFT -- the clock
**             included. Repainting at the clock's column left the pulled
**             copy's head behind: `IN> clear      hellihellish ...` after
**             one ↓, one more `hel` per edit.
**   erasure   hiding the clock cleared from its column to the end of the
**             row, and a recalled command that already reached past that
**             column lost its tail on screen -- the line you were about to
**             run was not the line you could see.
**
** Where the text ends is an UPPER bound (rl_text_end): clearing a few
** blank cells too far right costs nothing, clearing one too far left
** erases what was typed. */

/* The ghost (rl_suggest.c) goes where the cursor is, so the clock is
   judged against the end of the ghost and left out while the ghost reaches
   it; it comes back when the ghost is gone. Returns false when no ghost
   fits in the room left (a wide character at the very edge). */
static void	rp_ghost(t_rl *l, t_frame *f)
{
	int	w;

	f->end = rl_prompt_cols(l) + visible_width_cstr(rl_line_buffer);
	f->gn = rl_ghost_fit(f->ghost, f->cols - 1 - f->end, &w);
	f->show = rl_rp_fits(l, f->cols, f->end + w);
}

/* Paint what this frame needs, if anything: the ghost; or, without one,
   the clock shown or taken down, and a ghost that was up taken down. When
   the cursor may be off the prompt's row nothing is touched. */
static void	rp_paint(t_rl *l, t_frame *f)
{
	f->gn = 0;
	if (f->ghost)
		rp_ghost(l, f);
	if (!f->gn)
	{
		f->end = rl_text_col(l);
		if (f->end < 0 || f->end >= f->cols)
			return ;
		f->show = rl_rp_fits(l, f->cols, f->end);
		if (!f->show && !l->rp_painted && !l->sg_painted)
			return ;
	}
	rl_frame(l, f);
}

/* readline's redisplay, then ours. With no right prompt, no ghost and none
   painted last frame this is readline's redisplay and nothing else. */
static void	rp_redisplay(void)
{
	t_shell	*state;
	t_frame	f;
	int		rows;

	rl_redisplay();
	state = *zle_state_cell();
	if (!state)
		return ;
	rl_get_screen_size(&rows, &f.cols);
	f.ghost = rl_suggest_update(state, f.cols);
	if (!f.ghost && !state->rl.sg_painted && state->rl.rp_w <= 0)
		return ;
	rp_paint(&state->rl, &f);
}

/* Arm the painter for this readline() call. Nothing is painted until
   readline's first redisplay, which precedes the first keystroke, so the
   clock appears together with the prompt. It is needed with a right prompt
   or with the suggestion on; a widget that sets POSTDISPLAY brings it in
   by itself (rl_rprompt_hook). */
void	rl_rprompt_install(t_shell *state)
{
	state->rl.rp_painted = false;
	if (state->rl.rp_w > 0 || state->rl.feat_want[FEAT_ACCEPT])
		rl_rprompt_hook();
}

void	rl_rprompt_hook(void)
{
	rl_redisplay_function = rp_redisplay;
}
