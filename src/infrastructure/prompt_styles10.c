/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   prompt_styles10.c                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/03 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/03 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "prompt_styles.h"

/* Stack the four sprite rows, each followed by its slice of prompt info:
   row1 horns | row2 / eyes \ + user/cwd | row3 \ jaw / + branch |
   row4 forked tail + the burning fuse and the clock. */
static void	flame_body(t_string *r, t_prompt *p, size_t f, int st)
{
	devil_line1(r, f);
	vec_push_str(r, "\n");
	devil_line2(r, f, st);
	flame_info_user(r, p);
	vec_push_str(r, "\n");
	devil_line3(r, f, st);
	flame_info_branch(r, p);
	vec_push_str(r, "\n");
	devil_line4(r, f);
	flame_info_bar(r, p, f);
	vec_push_str(r, "\n");
}

/* "flame" (default): a big multi-line fire-devil sprite that lives above the
   input line - it breathes its own ember light, blinks, beams on success and
   scowls red on failure, while a spark crawls along the fuse beneath it. */
void	style_flame(size_t frame, int status, t_string *ret)
{
	t_prompt	p;

	gather_info(&p, frame);
	vec_push_ansi(ret, CUR_BEAM);
	flame_body(ret, &p, frame, status);
	if (status == 0)
		vec_push_str(ret, "\001\033[1;38;5;208m\002");
	else
		vec_push_str(ret, "\001\033[1;38;5;196m\002");
	vec_push_str(ret, "\xe2\x9d\xaf " A_RST);
	free_info(&p);
}
