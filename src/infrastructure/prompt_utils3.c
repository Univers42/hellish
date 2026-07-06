/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   prompt_utils3.c                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 15:18:14 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/02 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "prompt_private.h"

/* Subset of glyphs reused here from prompt_utils.c. Defined again rather than
   shared through a header so each translation unit is self-contained; the
   compiler merges identical string literals anyway. */
#define G_H   "\xe2\x94\x80"
#define G_TR  "\xe2\x95\xae"
#define G_BL  "\xe2\x95\xb0"
#define G_ARR "\xe2\x9d\xaf"

#define C_BOX  "\033[38;5;240m"
#define C_TIME "\033[38;5;240m"
#define C_OK   "\033[1m\033[38;5;76m"
#define C_ERR  "\033[1m\033[38;5;203m"
#define C_RST  "\033[0m"

/* Push the elastic spacer between the left segment (user/cwd/branch) and the
   right clock: a single \x1e marker in the frame colour. The readline child
   substitutes the actual box-drawing ─ run for the CURRENT terminal width when
   it prints the header (rl_header.c) -- and re-substitutes on every resize, so
   the header reflows live on zoom instead of wrapping or falling short. */
static void	push_fill(t_string *ret)
{
	vec_push_str(ret, " ");
	vec_push_ansi(ret, C_BOX);
	vec_push_str(ret, "\x1e");
	vec_push_ansi(ret, C_RST);
	vec_push_str(ret, " ");
}

/* Close the top row with the clock and then emit the second row (the arrow).
   The fill between content and clock is an elastic marker (see push_fill), so
   its width is decided by the readline child at print/resize time, not here.
   The arrow colour is green (C_OK) on exit 0, red (C_ERR) otherwise. */
void	prompt_time_and_pad(t_string *ret, t_prompt *p)
{
	get_timebuf(p->time_buf, sizeof(p->time_buf));
	push_fill(ret);
	vec_push_ansi(ret, C_TIME);
	vec_push_str(ret, p->time_buf);
	vec_push_ansi(ret, C_RST);
	vec_push_str(ret, " ");
	vec_push_ansi(ret, C_BOX);
	vec_push_str(ret, G_H G_TR);
	vec_push_ansi(ret, C_RST);
	vec_push_str(ret, "\n");
	vec_push_ansi(ret, C_BOX);
	vec_push_str(ret, G_BL G_H);
	vec_push_ansi(ret, C_RST);
	if (p->exit_status == 0)
		vec_push_ansi(ret, C_OK);
	else
		vec_push_ansi(ret, C_ERR);
	vec_push_str(ret, G_ARR " ");
	vec_push_ansi(ret, C_RST);
}
