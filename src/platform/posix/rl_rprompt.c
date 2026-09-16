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
** a jump-to-column on the wrong row would clear the user's own text. */

/* True while the cursor is certainly on the row the prompt ends on: the
   line holds no newline, does not wrap, and no search prompt has taken
   the row over. rl_end counts bytes, which is at least the columns a
   UTF-8 line takes, so this errs towards hands-off, never overlap. */
static bool	rp_on_prompt_row(t_rl *l, int cols)
{
	if (RL_ISSTATE(RL_STATE_ISEARCH) || RL_ISSTATE(RL_STATE_NSEARCH))
		return (false);
	if (rl_line_buffer && ft_strchr(rl_line_buffer, '\n'))
		return (false);
	return (l->rp_prompt_w + rl_end < cols);
}

/* Whether the clock has room to the right of the text, one blank between. */
static bool	rp_fits(t_rl *l, int cols)
{
	if (l->rp_w <= 0 || cols <= l->rp_w + 1)
		return (false);
	return (l->rp_prompt_w + rl_end + 1 < cols - l->rp_w);
}

/* One frame, one write: save, jump to the clock's column, then either the
   clock or a clear-to-end-of-line, then restore. */
static void	rp_frame(t_rl *l, int cols, bool show)
{
	t_string	f;
	char		*col;

	col = ft_itoa(cols - l->rp_w + 1);
	if (!col)
		return ;
	vec_init(&f);
	f.elem_size = 1;
	vec_push_str(&f, "\0337\033[");
	vec_push_str(&f, col);
	vec_push_str(&f, "G");
	if (show)
		vec_push_str(&f, (char *)l->rp_txt.ctx);
	else
		vec_push_str(&f, "\033[K");
	vec_push_str(&f, "\0338");
	tty_write_all(fileno(rl_outstream), f.ctx, f.len);
	xfree(f.ctx);
	xfree(col);
	l->rp_painted = show;
}

/* readline's redisplay, then ours. */
static void	rp_redisplay(void)
{
	t_shell	*state;
	int		rows;
	int		cols;

	rl_redisplay();
	state = *zle_state_cell();
	if (!state || state->rl.rp_w <= 0)
		return ;
	rl_get_screen_size(&rows, &cols);
	if (!rp_on_prompt_row(&state->rl, cols))
		return ;
	if (rp_fits(&state->rl, cols))
		rp_frame(&state->rl, cols, true);
	else if (state->rl.rp_painted)
		rp_frame(&state->rl, cols, false);
}

/* Arm the painter for this readline() call. Nothing is painted until
   readline's first redisplay, which precedes the first keystroke, so the
   clock appears together with the prompt. */
void	rl_rprompt_install(t_shell *state)
{
	state->rl.rp_painted = false;
	rl_redisplay_function = rp_redisplay;
}
