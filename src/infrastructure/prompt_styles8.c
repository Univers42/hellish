/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   prompt_styles8.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/03 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/03 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "prompt_styles.h"

/* The imp's eyes (3 visible cols): wide-eyed when idle, blinks now and then,
   beams ^w^ after a success and scowls >w< after a failed command. */
static const char	*imp_eyes(size_t f, int st)
{
	if (st != 0)
		return (">\xcf\x89<");
	if (f % 31 < 2)
		return ("-\xcf\x89-");
	if (f % 17 == 8)
		return ("^\xcf\x89^");
	return ("\xe2\x97\x95\xcf\x89\xe2\x97\x95");
}

/* The face colour: ember-flicker while idle, steady amber on a blink and a
   hard red while the imp is scowling at a failed command. */
static int	imp_color(size_t f, int st)
{
	if (st != 0)
		return (196);
	if (f % 31 < 2)
		return (214);
	return (fire_hue(f));
}

/* Line 1: the two horns, each flickering on its own phase. (14 visible cols) */
void	devil_line1(t_string *ret, size_t f)
{
	push_fg(ret, fire_hue(f));
	vec_push_str(ret, "    \xe2\x97\xa2\xe2\x97\xa3");
	push_fg(ret, fire_hue(f + 4));
	vec_push_str(ret, "  \xe2\x97\xa2\xe2\x97\xa3");
	vec_push_str(ret, A_RST "    ");
}

/* Line 2: the upper face  / EYES \  with the live expression. (14 cols) */
void	devil_line2(t_string *ret, size_t f, int st)
{
	push_fg(ret, 130);
	vec_push_str(ret, "   \xe2\x95\xb1 ");
	push_fg(ret, imp_color(f, st));
	vec_push_str(ret, (char *)imp_eyes(f, st));
	push_fg(ret, 130);
	vec_push_str(ret, " \xe2\x95\xb2");
	vec_push_str(ret, A_RST "    ");
}

/* Line 3: the lower face  \ \v/ /  (jaw / little fanged grin). (14 cols) */
void	devil_line3(t_string *ret, size_t f, int st)
{
	push_fg(ret, 130);
	vec_push_str(ret, "   \xe2\x95\xb2 ");
	push_fg(ret, imp_color(f, st));
	vec_push_str(ret, "\xe2\x95\xb2\xe2\x96\xbd\xe2\x95\xb1");
	push_fg(ret, 130);
	vec_push_str(ret, " \xe2\x95\xb1");
	vec_push_str(ret, A_RST "    ");
}
