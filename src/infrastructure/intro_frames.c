/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   intro_frames.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/03 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/03 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "header.h"

/* The hellish mascot: a little dinosaur that breathes fire. Stored as plain
   line-art (pure ASCII, so it reads as a dino on any font) and coloured at
   runtime by role -- body green, eye white, flame orange. Only the head row
   changes between frames, so the body lives in shared macros. */
#define DINO_TOP "               __"
#define DINO_B1 "     _.----._/ /"
#define DINO_B2 "    /         /"
#define DINO_B3 " __/ (  | (  |"
#define DINO_B4 "/__.-'|_|--|_|"
#define HEAD_IDLE "              /o_)"
#define HEAD_BLINK "              / _)"
#define HEAD_F1 "              /o_) ~"
#define HEAD_F2 "              /o_) ~~"
#define HEAD_F3 "              /o_) ~*~"

static const char	*g_idle[] = {
	DINO_TOP, HEAD_IDLE, DINO_B1, DINO_B2, DINO_B3, DINO_B4, NULL};
static const char	*g_fire1[] = {
	DINO_TOP, HEAD_F1, DINO_B1, DINO_B2, DINO_B3, DINO_B4, NULL};
static const char	*g_fire2[] = {
	DINO_TOP, HEAD_F2, DINO_B1, DINO_B2, DINO_B3, DINO_B4, NULL};
static const char	*g_fire3[] = {
	DINO_TOP, HEAD_F3, DINO_B1, DINO_B2, DINO_B3, DINO_B4, NULL};
static const char	*g_blink[] = {
	DINO_TOP, HEAD_BLINK, DINO_B1, DINO_B2, DINO_B3, DINO_B4, NULL};

/* The frame table: idle, a breath that flares up then dies down, then a blink.
   `count` receives the number of frames. */
const char	***intro_frames(size_t *count)
{
	static const char	**frames[] = {
		g_idle, g_fire1, g_fire2, g_fire3, g_fire2, g_fire1, g_blink, g_idle};

	if (count)
		*count = sizeof(frames) / sizeof(frames[0]);
	return (frames);
}
