/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   mascot_anim.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/03 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/03 13:00:20 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "prompt_private.h"

/* The animation frame: stamped in prompt_normal, advanced on each idle tick
   of the line reader, so the mascot keeps blinking from where it was. */
size_t	*anim_frame(void)
{
	static size_t	frame;

	return (&frame);
}

/* The last exit status, shared the same way, so the mascot keeps its mood. */
int	*anim_status(void)
{
	static int	status;

	return (&status);
}

/* The repaint climbs count_nl rows up from the cursor, so it is only
   safe while the cursor provably sits on the input's FIRST screen row.
   Walk the line up to rl_point in display columns (mbrtowc+wcwidth --
   rl_point is BYTES, and a pasted ✦ is 3 bytes for 1 column): any
   control byte (a pasted newline occupies a whole row), decode error,
   or a line already wide enough to wrap means the cursor row is not
   ours to predict -- report unsafe and let the caller skip the tick.
   Getting this wrong is not cosmetic: an undershoot lands the repaint
   ON the input row and ESC[2K eats the user's paste.

   mbrtowc's error returns are spelled out as (size_t)-1 / (size_t)-2
   rather than tested with `n > MB_CUR_MAX`. The short form is the same
   test on glibc, where MB_CUR_MAX is size_t -- and a -Werror=sign-compare
   build failure on Darwin, where it is an int. That is the whole of the
   macOS bug; the long form is also what the rest of the tree uses. */
static int	anim_line_fits(void)
{
	mbstate_t	st;
	wchar_t		wc;
	size_t		n;
	int			i;
	int			w;

	i = 0;
	w = anim_cells()->last_w;
	ft_memset(&st, 0, sizeof(st));
	while (i < rl_point)
	{
		if ((unsigned char)rl_line_buffer[i] < ' ')
			return (0);
		n = mbrtowc(&wc, rl_line_buffer + i, MB_CUR_MAX, &st);
		if (n == (size_t) - 1 || n == (size_t) - 2 || n == 0)
			return (0);
		if (wcwidth(wc) < 0)
			return (0);
		w += wcwidth(wc);
		i += (int)n;
	}
	return (w < get_cols());
}

/* Idle tick (~100ms, while the shell waits at the prompt; the line
   reader's timeout, rl_idle.c): advance the frame counter and repaint the
   rows above the input with the next pre-rendered variant. No allocation,
   no t_shell access -- the variants were fully rendered when the prompt
   was. Skipped whenever the cursor may have left the input's first row
   (wrapped/multiline/pasted input, isearch, completion): a frozen glyph is
   invisible, a repaint one row off destroys the line. */
void	anim_tick(void)
{
	t_panim		*a;
	t_string	view;

	a = anim_cells();
	if (a->count <= 0)
		return ;
	if ((rl_readline_state & (RL_STATE_ISEARCH | RL_STATE_NSEARCH
				| RL_STATE_COMPLETING)) || !anim_line_fits())
		return ;
	(*anim_frame())++;
	view = (t_string){0};
	view.ctx = a->buf[*anim_frame() % a->count];
	redraw_mascot(&view);
}

/* Frame variants exist only when PS1 contains \A and HELLISH_ANIM selects a
   live style; a static prompt never ticks and pays nothing. */
bool	anim_armed(void)
{
	return (anim_cells()->count > 0);
}
