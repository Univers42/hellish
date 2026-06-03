/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   prompt_glow2.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/03 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/03 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "prompt_styles.h"

/* A little star that breathes through warm tones, frame by frame. */
static int	glow_spark(size_t f)
{
	static const int	pal[10] = {130, 166, 202, 208, 214,
		220, 214, 208, 202, 166};

	return (pal[f % 10]);
}

/* Left of the line: the breathing star, then who / where / which branch.
   Returns the visible width so the rule can be sized to fill the rest. */
static int	push_glow_left(t_string *ret, t_prompt *p, size_t frame)
{
	int	w;

	push_fg(ret, glow_spark(frame));
	vec_push_str(ret, "\xe2\x9c\xa6  ");
	push_fg(ret, 223);
	vec_push_str(ret, p->user);
	vec_push_str(ret, A_RST "  ");
	push_fg(ret, 180);
	vec_push_str(ret, p->short_cwd);
	vec_push_str(ret, A_RST);
	w = 3 + (int)ft_strlen(p->user) + 2 + (int)ft_strlen(p->short_cwd);
	if (!p->branch)
		return (w);
	vec_push_str(ret, "  ");
	push_fg(ret, 173);
	vec_push_str(ret, G_BRANCH);
	vec_push_str(ret, p->branch);
	vec_push_str(ret, A_RST);
	return (w + 2 + (int)ft_strlen(p->branch));
}

/* The gliding ray of light + the clock, filling the rest of the top line. */
static void	push_glow_fill(t_string *ret, t_prompt *p, size_t frame, int w)
{
	int	n;

	n = p->cols - w - (int)ft_strlen(p->time_buf) - 5;
	if (n < 6)
		n = 6;
	if (n > 60)
		n = 60;
	vec_push_str(ret, "  ");
	push_glow_bar(ret, frame, n);
	vec_push_str(ret, "  \001\033[38;5;240m\002");
	vec_push_str(ret, p->time_buf);
	vec_push_str(ret, A_RST "\n");
}

/* "glow" (default): a calm two-line prompt - a breathing star, your context,
   and a warm ray of light that glides along the rule. It keeps animating even
   while you type, and closes on a status-coloured arrow. */
void	style_glow(size_t frame, int status, t_string *ret)
{
	t_prompt	p;
	int			w;

	gather_info(&p, frame);
	vec_push_ansi(ret, CUR_BEAM);
	w = push_glow_left(ret, &p, frame);
	push_glow_fill(ret, &p, frame, w);
	if (status == 0)
		vec_push_str(ret, "\001\033[1;38;5;208m\002");
	else
		vec_push_str(ret, "\001\033[1;38;5;203m\002");
	vec_push_str(ret, "\xe2\x9d\xaf " A_RST);
	free_info(&p);
}
