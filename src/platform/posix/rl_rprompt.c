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
** Where the text ends is an UPPER bound (rp_text_end): clearing a few
** blank cells too far right costs nothing, clearing one too far left
** erases what was typed. */

/* The last column the line can occupy, or -1 when the cursor may not be
   on the prompt's row (a search prompt owns it, the line has a newline).
   Per byte: a tab takes at most 8 columns, a control character is drawn
   as ^X (2), a byte a non-UTF-8 locale prints as \ooo takes 4, and every
   other byte at most 1 -- a UTF-8 character never takes more columns than
   it has bytes. */
static int	rp_text_end(t_rl *l)
{
	unsigned char	c;
	int				end;
	int				i;

	if (RL_ISSTATE(RL_STATE_ISEARCH) || RL_ISSTATE(RL_STATE_NSEARCH))
		return (-1);
	end = l->rp_prompt_w;
	i = 0;
	while (rl_line_buffer && i < rl_end)
	{
		c = (unsigned char)rl_line_buffer[i++];
		if (c == '\n')
			return (-1);
		if (c == '\t')
			end += 8;
		else if (c < ' ' || c == 0x7f)
			end += 2;
		else if (c >= 0x80 && MB_CUR_MAX == 1)
			end += 4;
		else
			end += 1;
	}
	return (end);
}

/* Append CSI <n> G (cursor to column n). False if it could not be built,
   and then nothing may be cleared: the clear would land at the cursor. */
static bool	rp_push_col(t_string *f, int n)
{
	char	*s;

	s = ft_itoa(n);
	if (!s)
		return (false);
	vec_push_str(f, "\033[");
	vec_push_str(f, s);
	vec_push_str(f, "G");
	xfree(s);
	return (true);
}

/* One frame, one write: save, clear from just past the text to the end of
   the row, the clock if it is shown, restore. */
static void	rp_frame(t_rl *l, int cols, int end, bool show)
{
	t_string	f;

	vec_init(&f);
	f.elem_size = 1;
	vec_push_str(&f, "\0337");
	if (!rp_push_col(&f, end + 1))
		return (xfree(f.ctx));
	vec_push_str(&f, "\033[K");
	if (show && !rp_push_col(&f, cols - l->rp_w + 1))
		return (xfree(f.ctx));
	if (show)
		vec_push_str(&f, (char *)l->rp_txt.ctx);
	vec_push_str(&f, "\0338");
	tty_write_all(fileno(rl_outstream), f.ctx, f.len);
	xfree(f.ctx);
	l->rp_painted = show;
}

/* readline's redisplay, then ours. The clock is shown with one blank
   between it and the text; otherwise a painted one is taken down. */
static void	rp_redisplay(void)
{
	t_shell	*state;
	int		rows;
	int		cols;
	int		end;

	rl_redisplay();
	state = *zle_state_cell();
	if (!state || state->rl.rp_w <= 0)
		return ;
	rl_get_screen_size(&rows, &cols);
	end = rp_text_end(&state->rl);
	if (end < 0 || end >= cols)
		return ;
	if (cols > state->rl.rp_w + 1 && end + 1 < cols - state->rl.rp_w)
		rp_frame(&state->rl, cols, end, true);
	else if (state->rl.rp_painted)
		rp_frame(&state->rl, cols, end, false);
}

/* Arm the painter for this readline() call. Nothing is painted until
   readline's first redisplay, which precedes the first keystroke, so the
   clock appears together with the prompt. */
void	rl_rprompt_install(t_shell *state)
{
	state->rl.rp_painted = false;
	rl_redisplay_function = rp_redisplay;
}
