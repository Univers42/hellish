/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   prompt_glow.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/03 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/03 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "prompt_styles.h"

/* The bright point of the light, ping-ponging smoothly along a bar of width w
   (a triangle wave so it glides back and forth instead of jumping on wrap). */
int	glow_pos(size_t frame, int w)
{
	int	t;

	if (w < 2)
		return (0);
	t = (int)(frame % (size_t)(2 * w));
	if (t < w)
		return (t);
	return (2 * w - t);
}

/* Colour of a rule cell `d` cells from the light: a soft warm halo that fades
   from white-hot at the centre to a dim ember on the rest of the line. */
int	glow_cell(int d)
{
	if (d < 0)
		d = -d;
	if (d == 0)
		return (231);
	if (d == 1)
		return (223);
	if (d == 2)
		return (216);
	if (d == 3)
		return (209);
	if (d == 4)
		return (130);
	return (238);
}

/* The animated rule: a warm ray of light gliding along a row of dashes. */
void	push_glow_bar(t_string *ret, size_t frame, int n)
{
	int	i;
	int	pos;

	pos = glow_pos(frame, n);
	i = -1;
	while (++i < n)
	{
		push_fg(ret, glow_cell(i - pos));
		vec_push_str(ret, G_DASH);
	}
}
