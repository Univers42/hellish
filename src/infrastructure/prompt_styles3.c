/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   prompt_styles3.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/03 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/03 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "prompt_styles.h"

/* A live audio-waveform of block glyphs whose phase shifts left each prompt
   (precomputed heights -> no libm); each bar tinted by the gradient. */
static void	push_wave(t_string *ret, size_t f, int n)
{
	static const int	h[16] = {0, 1, 2, 4, 5, 7, 8, 8, 7, 5, 4,
		2, 1, 0, 0, 1};
	static const char	*blk[9] = {" ", "\xe2\x96\x81", "\xe2\x96\x82",
		"\xe2\x96\x83", "\xe2\x96\x84", "\xe2\x96\x85", "\xe2\x96\x86",
		"\xe2\x96\x87", "\xe2\x96\x88"};
	int					i;

	i = 0;
	while (i < n)
	{
		push_fg(ret, aurora_hue(f, i));
		vec_push_str(ret, (char *)blk[h[(i + (int)f) % 16]]);
		i++;
	}
}

static void	push_status_arrow(t_string *ret, int status)
{
	if (status == 0)
		vec_push_str(ret, "\001\033[1;38;5;76m\002");
	else
		vec_push_str(ret, "\001\033[1;38;5;203m\002");
	vec_push_str(ret, G_ARROW " " A_RST);
}

/* "wave": user + cwd, then an animated waveform rule, then the status arrow. */
void	style_wave(t_shell *state, t_string *ret)
{
	t_prompt	p;
	int			n;

	gather_info(&p, state->prompt_frame);
	vec_push_ansi(ret, CUR_BEAM);
	push_fg(ret, 117);
	vec_push_str(ret, p.user);
	vec_push_str(ret, A_RST " ");
	push_fg(ret, 81);
	vec_push_str(ret, p.short_cwd);
	vec_push_str(ret, A_RST " ");
	n = p.cols - (int)ft_strlen(p.user) - (int)ft_strlen(p.short_cwd) - 3;
	if (n < 4)
		n = 4;
	if (n > 64)
		n = 64;
	push_wave(ret, state->prompt_frame, n);
	vec_push_str(ret, A_RST "\n");
	push_status_arrow(ret, p.exit_status);
	free_info(&p);
}

/* "pulse": ultra-minimal single line - a breathing orb, the cwd, the arrow. */
void	style_pulse(t_shell *state, t_string *ret)
{
	static const char	*orb[8] = {"\xe2\x97\x8b", "\xe2\x97\x94",
		"\xe2\x97\x91", "\xe2\x97\x95", "\xe2\x97\x8f", "\xe2\x97\x95",
		"\xe2\x97\x91", "\xe2\x97\x94"};
	t_prompt			p;

	gather_info(&p, state->prompt_frame);
	vec_push_ansi(ret, CUR_BEAM);
	push_fg(ret, aurora_hue(state->prompt_frame, 0));
	vec_push_str(ret, (char *)orb[state->prompt_frame % 8]);
	vec_push_str(ret, A_RST " ");
	push_fg(ret, 75);
	vec_push_str(ret, p.short_cwd);
	vec_push_str(ret, A_RST " ");
	push_status_arrow(ret, p.exit_status);
	free_info(&p);
}
