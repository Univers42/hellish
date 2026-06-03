/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   mascot_anim.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/03 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/03 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "prompt_private.h"

/* The animation frame, shared parent -> readline-child by fork inheritance:
   the parent stamps it in prompt_normal, the child advances its own copy each
   idle tick so the mascot keeps blinking from where the prompt started. */
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

/* readline idle tick (~90ms, even while typing): advance a frame, re-render
   the prompt and repaint the mascot line above the cursor. */
int	mascot_hook(void)
{
	t_string	r;

	(*anim_frame())++;
	vec_init(&r);
	r.elem_size = 1;
	render_prompt(&r, *anim_frame(), *anim_status());
	vec_push_char(&r, 0);
	redraw_mascot(&r);
	free(r.ctx);
	return (0);
}

/* Arm the idle animation in the readline child (poll every ~90ms). Opt out
   with HELLISH_NO_MASCOT, which leaves a plain, perfectly static prompt. */
void	mascot_install(void)
{
	if (getenv("HELLISH_NO_MASCOT"))
		return ;
	rl_event_hook = mascot_hook;
	rl_set_keyboard_input_timeout(90000);
}
