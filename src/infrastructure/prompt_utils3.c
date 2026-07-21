/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   prompt_utils3.c                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 15:18:14 by dlesieur          #+#    #+#             */
/*   Updated: 2026/07/21 00:00:00 by dlesieur         ###   ########.fr       */
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

#define C_RST "\033[0m"

/* One step of the fill gradient: ember (132,62,46) at the left fading into
   the frame grey (74,79,87) at the clock — the "hellish" signature, kept
   dark enough to read as a rule line, not a rainbow. */
static void	grad_seq(char *buf, size_t n, int k, int count)
{
	int	r;
	int	g;
	int	b;

	if (count < 2)
		count = 2;
	r = 132 + (74 - 132) * k / (count - 1);
	g = 62 + (79 - 62) * k / (count - 1);
	b = 46 + (87 - 46) * k / (count - 1);
	snprintf(buf, n, "\033[38;2;%d;%d;%dm", r, g, b);
}

/* The ─ spacer between the left segments and the right clock: a per-char
   ember gradient on truecolor terminals, the flat frame grey elsewhere.
   Leading and trailing plain spaces on both variants. */
static void	push_fill(t_string *ret, int count)
{
	char	seq[48];
	int		k;

	vec_push_str(ret, " ");
	if (!pal_truecolor())
	{
		vec_push_ansi(ret, pal(PAL_BOX));
		while (count-- > 0)
			vec_push_str(ret, G_H);
	}
	else
	{
		k = 0;
		while (k < count)
		{
			grad_seq(seq, sizeof(seq), k, count);
			vec_push_ansi(ret, seq);
			vec_push_str(ret, G_H);
			k++;
		}
	}
	vec_push_ansi(ret, C_RST);
	vec_push_str(ret, " ");
}

/* Close the top row with the clock: jobs/duration extras, the ─ fill,
   the time, and the top-right corner. The fill width is computed as:
   terminal columns − visible content already accumulated − clock width −
   2 border chars. A minimum of 3 is enforced so the box never collapses
   to zero on very narrow terminals. */
void	prompt_time_and_pad(t_string *ret, t_prompt *p)
{
	int	right_w;

	render_extras(ret, p);
	get_timebuf(p->time_buf, sizeof(p->time_buf));
	right_w = (int)ft_strlen(p->time_buf) + 3;
	p->pad = p->cols - p->vis_w - right_w - 2;
	if (p->pad < 3)
		p->pad = 3;
	push_fill(ret, p->pad - 2);
	vec_push_ansi(ret, pal(PAL_TIME));
	vec_push_str(ret, p->time_buf);
	vec_push_ansi(ret, C_RST);
	vec_push_str(ret, " ");
	vec_push_ansi(ret, pal(PAL_BOX));
	vec_push_str(ret, G_H G_TR);
	vec_push_ansi(ret, C_RST);
	vec_push_str(ret, "\n");
	prompt_arrow_row(ret, p);
}

/* The second row: "╰─❯ " with a green arrow after success, or
   "╰─ ✘N ❯ " with the failing exit status spelled out in red — the
   number matters more than the colour when you scroll back through a
   long session hunting for the command that broke. */
void	prompt_arrow_row(t_string *ret, t_prompt *p)
{
	char	buf[16];

	vec_push_ansi(ret, pal(PAL_BOX));
	vec_push_str(ret, G_BL G_H);
	vec_push_ansi(ret, C_RST);
	if (p->exit_status != 0)
	{
		snprintf(buf, sizeof(buf), " \xe2\x9c\x98%d", p->exit_status);
		vec_push_ansi(ret, pal(PAL_ERR));
		vec_push_str(ret, buf);
		vec_push_str(ret, " " G_ARR " ");
		vec_push_ansi(ret, C_RST);
		return ;
	}
	vec_push_ansi(ret, pal(PAL_OK));
	vec_push_str(ret, G_ARR " ");
	vec_push_ansi(ret, C_RST);
}
