/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   prompt_styles7.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/03 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/03 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "prompt_styles.h"

/* warm user / cwd / branch segments to the right of the imp; returns width. */
static int	push_ember_left(t_string *ret, t_prompt *p)
{
	int	w;

	push_fg(ret, 223);
	vec_push_str(ret, p->user);
	vec_push_str(ret, A_RST "  ");
	push_fg(ret, 180);
	vec_push_str(ret, p->short_cwd);
	vec_push_str(ret, A_RST);
	w = (int)ft_strlen(p->user) + 2 + (int)ft_strlen(p->short_cwd);
	if (!p->branch)
		return (w);
	vec_push_str(ret, "  ");
	push_fg(ret, 173);
	vec_push_str(ret, G_BRANCH);
	vec_push_str(ret, p->branch);
	vec_push_str(ret, A_RST);
	return (w + 3 + (int)ft_strlen(p->branch));
}

/* The fuse width left between the info and the clock. */
static int	ember_fuse_width(t_prompt *p, int w)
{
	int	n;

	n = p->cols - w - (int)ft_strlen(p->time_buf) - 6;
	if (n < 6)
		n = 6;
	if (n > 60)
		n = 60;
	return (n);
}

/* "ember" (default): the hellish fire imp + a burning-fuse rule whose spark
   creeps toward your prompt - the brand, alive and idling on its own. */
void	style_ember(size_t frame, int status, t_string *ret)
{
	t_prompt	p;
	int			w;

	gather_info(&p, frame);
	vec_push_ansi(ret, CUR_BEAM);
	push_imp(ret, frame, status);
	vec_push_str(ret, "  ");
	w = push_ember_left(ret, &p) + 7;
	vec_push_str(ret, "  ");
	fuse_bar(ret, frame, ember_fuse_width(&p, w));
	vec_push_str(ret, "  \001\033[38;5;240m\002");
	vec_push_str(ret, p.time_buf);
	vec_push_str(ret, A_RST "\n");
	if (status == 0)
		vec_push_str(ret, "\001\033[1;38;5;208m\002");
	else
		vec_push_str(ret, "\001\033[1;38;5;196m\002");
	vec_push_str(ret, "\xe2\x9d\xaf " A_RST);
	free_info(&p);
}
