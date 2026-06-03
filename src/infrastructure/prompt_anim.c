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
	return (style == STYLE_FLAME || style == STYLE_EMBER
		|| style == STYLE_BREATHE || style == STYLE_AURORA
		|| style == STYLE_WAVE || style == STYLE_POWERLINE);
}

/* Render the full prompt for (style, frame); pass the remembered status so the
   ember mascot keeps its mood while idle. */
void	render_top(int style, size_t frame, t_string *ret)
{
	int	st;

	st = *anim_status();
	if (style == STYLE_FLAME)
		style_flame(frame, st, ret);
	else if (style == STYLE_WAVE)
		style_wave(frame, st, ret);
	else if (style == STYLE_POWERLINE)
		style_powerline(frame, st, ret);
	else if (style == STYLE_AURORA)
		style_aurora(frame, st, ret);
	else if (style == STYLE_EMBER)
		style_ember(frame, st, ret);
	else
		style_breathe(frame, st, ret);
}
