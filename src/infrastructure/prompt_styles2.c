/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   prompt_styles2.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/03 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/03 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "prompt_styles.h"

/* Pick the prompt style from $HELLISH_PROMPT_STYLE (default: glow). */
int	select_prompt_style(void)
{
	char	*s;

	s = getenv("HELLISH_PROMPT_STYLE");
	if (!s)
		return (STYLE_GLOW);
	if (!ft_strcmp(s, "glow"))
		return (STYLE_GLOW);
	if (!ft_strcmp(s, "breathe"))
		return (STYLE_BREATHE);
	if (!ft_strcmp(s, "aurora"))
		return (STYLE_AURORA);
	if (!ft_strcmp(s, "wave"))
		return (STYLE_WAVE);
	if (!ft_strcmp(s, "pulse"))
		return (STYLE_PULSE);
	if (!ft_strcmp(s, "powerline"))
		return (STYLE_POWERLINE);
	if (!ft_strcmp(s, "classic"))
		return (STYLE_CLASSIC);
	return (STYLE_GLOW);
}

/* Left part of the aurora prompt: spinner, user, cwd and (if any) git branch.
   Returns the visible column width consumed, for right-side padding. */
static int	push_aurora_left(t_string *ret, t_prompt *p, size_t frame)
{
	int	w;

	push_spinner(ret, frame);
	push_fg(ret, aurora_hue(frame, 0));
	vec_push_str(ret, " ");
	vec_push_str(ret, p->user);
	vec_push_str(ret, A_RST " ");
	push_fg(ret, 75);
	vec_push_str(ret, p->short_cwd);
	vec_push_str(ret, A_RST);
	w = 2 + (int)ft_strlen(p->user) + 1 + (int)ft_strlen(p->short_cwd);
	if (!p->branch)
		return (w);
	vec_push_str(ret, " ");
	push_fg(ret, 114);
	vec_push_str(ret, G_BRANCH);
	vec_push_str(ret, p->branch);
	vec_push_str(ret, A_RST);
	return (w + 2 + (int)ft_strlen(p->branch));
}

/* The flowing gradient rule + clock that fills the rest of the top line. */
static void	push_aurora_fill(t_string *ret, t_prompt *p, size_t frame, int w)
{
	int	fill;
	int	i;

	fill = p->cols - w - (int)ft_strlen(p->time_buf) - 4;
	if (fill < 2)
		fill = 2;
	vec_push_str(ret, " ");
	i = 0;
	while (i < fill)
	{
		push_fg(ret, aurora_hue(frame, i));
		vec_push_str(ret, G_DASH);
		i++;
	}
	vec_push_str(ret, " \001\033[38;5;244m\002");
	vec_push_str(ret, p->time_buf);
	vec_push_str(ret, A_RST "\n");
}

/* "aurora" (default): an animated braille spinner, segmented info and a colour
   gradient that drifts left each prompt, closed by a status-coloured arrow. */
void	style_aurora(size_t frame, int status, t_string *ret)
{
	t_prompt	p;
	int			w;

	gather_info(&p, frame);
	vec_push_ansi(ret, CUR_BEAM);
	w = push_aurora_left(ret, &p, frame);
	push_aurora_fill(ret, &p, frame, w);
	if (status == 0)
		vec_push_str(ret, "\001\033[1;38;5;76m\002");
	else
		vec_push_str(ret, "\001\033[1;38;5;203m\002");
	vec_push_str(ret, G_ARROW " " A_RST);
	free_info(&p);
}
