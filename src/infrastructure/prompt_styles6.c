/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   prompt_styles6.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/03 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/03 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "prompt_styles.h"

/* The hellish mascot's flicker: a warm, slightly irregular fire palette. */
static int	fire_hue(size_t f)
{
	static const int	pal[12] = {208, 214, 202, 220, 208, 196,
		214, 209, 202, 214, 220, 208};

	return (pal[f % 12]);
}

/* The little fire imp (◣ω◢): flickers like an ember when idle, blinks now
   and then, beams (^ω^) on success and scowls (>ω<) bright-red on a fail. */
void	push_imp(t_string *ret, size_t frame, int status)
{
	if (status != 0)
	{
		push_fg(ret, 196);
		vec_push_str(ret, "(>\xcf\x89<)");
	}
	else if (frame % 27 < 2)
	{
		push_fg(ret, 214);
		vec_push_str(ret, "(-\xcf\x89-)");
	}
	else if (frame % 13 == 6)
	{
		push_fg(ret, fire_hue(frame));
		vec_push_str(ret, "(^\xcf\x89^)");
	}
	else
	{
		push_fg(ret, fire_hue(frame));
		vec_push_str(ret, "(\xe2\x97\xa3\xcf\x89\xe2\x97\xa2)");
	}
	vec_push_str(ret, A_RST);
}

/* Colour of one fuse cell by its distance `d` behind the travelling spark. */
static int	fuse_color(int d, int spark)
{
	if (spark)
		return (231);
	if (d == 1)
		return (220);
	if (d == 2)
		return (208);
	if (d == 3)
		return (202);
	if (d >= 4 && d <= 7)
		return (88);
	return (237);
}

/* A burning fuse: a bright spark ● races left->right leaving a hot, fading
   trail, then re-ignites at the start - forever creeping toward the prompt. */
void	fuse_bar(t_string *ret, size_t frame, int n)
{
	int	i;
	int	pos;

	pos = (int)(frame % (size_t)n);
	i = -1;
	while (++i < n)
	{
		push_fg(ret, fuse_color(pos - i, i == pos));
		if (i == pos)
			vec_push_str(ret, "\xe2\x97\x8f");
		else
			vec_push_str(ret, G_DASH);
	}
}
