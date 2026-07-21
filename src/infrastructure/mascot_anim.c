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

/* readline idle tick (~100ms, while the shell waits at the prompt):
   advance the frame counter and repaint the rows above the input with
   the next pre-rendered variant. No allocation, no t_shell access —
   the variants were fully rendered by the parent before the fork. */
int	mascot_hook(void)
{
	t_panim		*a;
	t_string	view;

	a = anim_cells();
	if (a->count <= 0)
		return (0);
	(*anim_frame())++;
	view = (t_string){0};
	view.ctx = a->buf[*anim_frame() % a->count];
	redraw_mascot(&view);
	return (0);
}

/* Install the idle hook only when frame variants exist (PS1 contains \A
   and HELLISH_ANIM selects a live style); a static prompt keeps the
   hook NULL and pays nothing. */
void	mascot_install(void)
{
	if (anim_cells()->count > 0)
		rl_event_hook = mascot_hook;
	else
		rl_event_hook = NULL;
}
