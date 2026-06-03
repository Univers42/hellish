/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   prompt_anim2.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/03 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/03 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "prompt_styles.h"

/* Last command status, shared parent -> child (fork) so the mascot keeps its
   mood (e.g. angry after a failure) while the prompt idles. */
int	*anim_status(void)
{
	static int	status;

	return (&status);
}

/* readline idle tick: while the input line is empty, advance one frame and
   repaint the top line; while the user is typing (rl_end > 0) we leave the
   screen alone so a wrapped input line can never be corrupted. */
int	anim_hook(void)
{
	t_string	r;

	if (rl_end > 0)
		return (0);
	(*anim_frame())++;
	vec_init(&r);
	r.elem_size = 1;
	render_top(*anim_style(), *anim_frame(), &r);
	vec_push_char(&r, 0);
	redraw_top(&r);
	free(r.ctx);
	return (0);
}

/* Arm the idle animation in the readline child: poll for a keypress every ~90ms
   and, when none comes, run anim_hook. Single-line styles are left static. */
void	prompt_anim_install(int style)
{
	if (!style_has_top_line(style))
		return ;
	*anim_style() = style;
	rl_event_hook = anim_hook;
	rl_set_keyboard_input_timeout(90000);
}
