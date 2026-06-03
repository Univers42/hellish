/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   prompt_anim.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/03 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/03 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "prompt_styles.h"

/* The animation frame, shared parent -> readline-child via fork inheritance:
   the parent stamps it in prompt_normal, the child advances its own copy each
   idle tick, so the animation continues where the prompt started. */
size_t	*anim_frame(void)
{
	static size_t	frame;

	return (&frame);
}

/* The style being animated (set by prompt_anim_install in the child). */
int	*anim_style(void)
{
	static int	style;

	return (&style);
}

/* Only the multi-line styles have a static top line that we can animate in
   place without disturbing readline's (bottom) input line. */
int	style_has_top_line(int style)
{
	return (style == STYLE_BREATHE || style == STYLE_AURORA
		|| style == STYLE_WAVE || style == STYLE_POWERLINE);
}

/* Render the full prompt for (style, frame); the arrow status is irrelevant to
   the top line we redraw, so pass 0. */
void	render_top(int style, size_t frame, t_string *ret)
{
	if (style == STYLE_WAVE)
		style_wave(frame, 0, ret);
	else if (style == STYLE_POWERLINE)
		style_powerline(frame, 0, ret);
	else if (style == STYLE_AURORA)
		style_aurora(frame, 0, ret);
	else
		style_breathe(frame, 0, ret);
}

/* Repaint just the top line in place: hardware-save the cursor, step up to the
   line above the input, clear it, print the top line (minus \001/\002 markers),
   then restore the cursor exactly where readline left it. */
void	redraw_top(t_string *r)
{
	char	*nl;
	size_t	i;

	nl = ft_strrchr((char *)r->ctx, '\n');
	if (!nl)
		return ;
	fputs("\0337\033[1A\r\033[2K", rl_outstream);
	i = 0;
	while ((char *)r->ctx + i < nl)
	{
		if (((char *)r->ctx)[i] != '\001' && ((char *)r->ctx)[i] != '\002')
			fputc((unsigned char)((char *)r->ctx)[i], rl_outstream);
		i++;
	}
	fputs("\0338", rl_outstream);
	fflush(rl_outstream);
}
