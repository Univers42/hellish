/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   prompt_styles.h                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/03 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/03 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef PROMPT_STYLES_H
# define PROMPT_STYLES_H

# include "prompt_private.h"

/* DECSCUSR cursor shapes (zero-width). */
# define CUR_BEAM "\033[6 q"
# define CUR_BLOCK "\033[2 q"
# define CUR_UNDER "\033[4 q"

/* glyphs (UTF-8) reused across styles */
# define G_ARROW "\xe2\x9d\xaf"
# define G_DASH "\xe2\x94\x80"
# define G_BRANCH "\xe2\x8e\x87"
# define A_RST "\001\033[0m\002"
# define PL_SEP "\xee\x82\xb0"

enum e_prompt_style
{
	STYLE_AURORA,
	STYLE_WAVE,
	STYLE_PULSE,
	STYLE_POWERLINE,
	STYLE_CLASSIC
};

/* shared helpers (prompt_styles.c) */
void	push_fg(t_string *ret, int color);
int		aurora_hue(size_t frame, int i);
void	gather_info(t_prompt *p, size_t frame);
void	free_info(t_prompt *p);
void	push_spinner(t_string *ret, size_t f);

/* style renderers; each builds a full (possibly multi-line) prompt into ret */
int		select_prompt_style(void);
void	style_aurora(t_shell *state, t_string *ret);
void	style_wave(t_shell *state, t_string *ret);
void	style_pulse(t_shell *state, t_string *ret);
void	style_powerline(t_shell *state, t_string *ret);

#endif
