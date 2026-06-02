/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   prompt_styles4.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/03 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/03 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "prompt_styles.h"

/* Open a segment: background `bg` with a bright foreground for the content. */
static void	push_seg(t_string *ret, int bg)
{
	char	*s;

	s = ft_itoa(bg);
	if (!s)
		return ;
	vec_push_char(ret, '\001');
	vec_push_str(ret, "\033[48;5;");
	vec_push_str(ret, s);
	vec_push_str(ret, ";38;5;231m");
	vec_push_char(ret, '\002');
	free(s);
}

/* Set only the background (bg < 0 => terminal default), for the angle glyph. */
static void	push_bg_only(t_string *ret, int bg)
{
	char	*s;

	vec_push_char(ret, '\001');
	if (bg < 0)
		vec_push_str(ret, "\033[49m");
	else
	{
		s = ft_itoa(bg);
		vec_push_str(ret, "\033[48;5;");
		vec_push_str(ret, s);
		vec_push_str(ret, "m");
		free(s);
	}
	vec_push_char(ret, '\002');
}

/* A bg-coloured segment then the  angle that blends into the next bg. */
static void	pl_segment(t_string *ret, const char *txt, int bg, int next_bg)
{
	push_seg(ret, bg);
	vec_push_str(ret, " ");
	vec_push_str(ret, (char *)txt);
	vec_push_str(ret, " ");
	push_fg(ret, bg);
	push_bg_only(ret, next_bg);
	vec_push_str(ret, PL_SEP);
}

/* "powerline": sleek angled segments (needs a Nerd Font for the  glyph). */
void	style_powerline(t_shell *state, t_string *ret)
{
	t_prompt	p;

	gather_info(&p, state->prompt_frame);
	vec_push_ansi(ret, CUR_BEAM);
	pl_segment(ret, p.user, 24, 31);
	pl_segment(ret, p.short_cwd, 31, 240);
	pl_segment(ret, p.time_buf, 240, -1);
	vec_push_str(ret, A_RST "\n");
	if (p.exit_status == 0)
		vec_push_str(ret, "\001\033[1;38;5;76m\002");
	else
		vec_push_str(ret, "\001\033[1;38;5;203m\002");
	vec_push_str(ret, G_ARROW " " A_RST);
	free_info(&p);
}
