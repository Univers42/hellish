/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   prompt_styles5.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/03 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/03 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "prompt_styles.h"

/* A slow, soft dim<->bright cyan breath (~1s/cycle at the 90ms tick). */
static int	breathe_hue(size_t f)
{
	static const int	pal[12] = {23, 30, 30, 37, 44, 51,
		51, 44, 37, 30, 30, 23};

	return (pal[f % 12]);
}

/* The breathing sparkle, then calm muted user / cwd / branch segments. */
static int	push_breathe_left(t_string *ret, t_prompt *p, size_t frame)
{
	int	w;

	push_fg(ret, breathe_hue(frame));
	vec_push_str(ret, "\xe2\x9c\xa6  ");
	push_fg(ret, 252);
	vec_push_str(ret, p->user);
	vec_push_str(ret, A_RST "  ");
	push_fg(ret, 110);
	vec_push_str(ret, p->short_cwd);
	vec_push_str(ret, A_RST);
	w = 3 + (int)ft_strlen(p->user) + 2 + (int)ft_strlen(p->short_cwd);
	if (!p->branch)
		return (w);
	vec_push_str(ret, "  ");
	push_fg(ret, 108);
	vec_push_str(ret, G_BRANCH);
	vec_push_str(ret, p->branch);
	vec_push_str(ret, A_RST);
	return (w + 3 + (int)ft_strlen(p->branch));
}

/* A thin, static (non-flowing) rule and a dim clock - everything here is calm;
   only the sparkle moves. */
static void	push_calm_fill(t_string *ret, t_prompt *p, int w)
{
	int	fill;
	int	i;

	fill = p->cols - w - (int)ft_strlen(p->time_buf) - 6;
	if (fill < 2)
		fill = 2;
	vec_push_str(ret, "  \001\033[38;5;237m\002");
	i = 0;
	while (i < fill)
	{
		vec_push_str(ret, G_DASH);
		i++;
	}
	vec_push_str(ret, "  \001\033[38;5;240m\002");
	vec_push_str(ret, p->time_buf);
	vec_push_str(ret, A_RST "\n");
}

/* "breathe" (default): a single gently-pulsing sparkle on an otherwise still
   line - quiet, elegant, never busy. */
void	style_breathe(size_t frame, int status, t_string *ret)
{
	t_prompt	p;
	int			w;

	gather_info(&p, frame);
	vec_push_ansi(ret, CUR_BEAM);
	w = push_breathe_left(ret, &p, frame);
	push_calm_fill(ret, &p, w);
	if (status == 0)
		vec_push_str(ret, "\001\033[38;5;108m\002");
	else
		vec_push_str(ret, "\001\033[38;5;203m\002");
	vec_push_str(ret, G_ARROW " " A_RST);
	free_info(&p);
}
