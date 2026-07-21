/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   prompt_palette.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/21 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/07/21 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "prompt_private.h"

/* Truecolor prompt palette (index order = enum e_pal): a Tokyo-Night-ish
   set — blue user, soft-blue path with a bright last component, green
   branch, amber accents, red for errors and root. Only emitted when the
   terminal advertises 24-bit colour via COLORTERM. */
static const char	*g_pal_tc[PAL_N] = {
	"\033[38;2;90;96;106m",
	"\033[1m\033[38;2;122;162;247m",
	"\033[1m\033[38;2;224;108;117m",
	"\033[38;2;108;114;125m",
	"\033[38;2;108;114;125m",
	"\033[38;2;108;142;191m",
	"\033[1m\033[38;2;158;203;255m",
	"\033[1m\033[38;2;152;195;121m",
	"\033[38;2;229;192;123m",
	"\033[38;2;229;192;123m",
	"\033[38;2;96;102;112m",
	"\033[1m\033[38;2;152;195;121m",
	"\033[1m\033[38;2;224;108;117m",
	"\033[1m\033[38;2;125;207;255m",
	"\033[38;2;229;192;123m",
};

/* 256-colour fallback, same slot order — the values the prompt has always
   used, so terminals without COLORTERM look exactly like before. */
static const char	*g_pal_256[PAL_N] = {
	"\033[38;5;240m",
	"\033[1m\033[38;5;81m",
	"\033[1m\033[38;5;203m",
	"\033[38;5;243m",
	"\033[38;5;243m",
	"\033[38;5;67m",
	"\033[1m\033[38;5;75m",
	"\033[1m\033[38;5;114m",
	"\033[38;5;179m",
	"\033[38;5;179m",
	"\033[38;5;240m",
	"\033[1m\033[38;5;76m",
	"\033[1m\033[38;5;203m",
	"\033[1m\033[38;5;110m",
	"\033[38;5;179m",
};

/* Truecolor capability, detected once per process: COLORTERM containing
   "truecolor" or "24bit" is the de-facto standard signal (VTE, kitty,
   iTerm2, Windows Terminal all set it). */
int	pal_truecolor(void)
{
	static int	mode = -1;
	char		*ct;

	if (mode >= 0)
		return (mode);
	mode = 0;
	ct = getenv("COLORTERM");
	if (ct && (ft_strstr(ct, "truecolor") || ft_strstr(ct, "24bit")))
		mode = 1;
	return (mode);
}

/* The one lookup every segment renderer goes through. */
const char	*pal(int id)
{
	if (id < 0 || id >= PAL_N)
		return ("");
	if (pal_truecolor())
		return (g_pal_tc[id]);
	return (g_pal_256[id]);
}
