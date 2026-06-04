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

/* The mascot is the 42 logo itself -- the exact art from the school's file
   header. The entrance reveals it line by line, then holds; it is coloured
   salmon at runtime (see rune_color), the pixels-to-characters idea from the
   GitHub Copilot CLI banner applied to a logo we did not have to draw. */
#define LOGO0 "        :::      ::::::::"
#define LOGO1 "      :+:      :+:    :+:"
#define LOGO2 "    +:+ +:+         +:+"
#define LOGO3 "  +#+  +:+       +#+"
#define LOGO4 "+#+#+#+#+#+   +#+"
#define LOGO5 "     #+#    #+#"
#define LOGO6 "    ###   ########.fr"

static const char	*g_r1[] = {LOGO0, NULL};
static const char	*g_r2[] = {LOGO0, LOGO1, NULL};
static const char	*g_r3[] = {LOGO0, LOGO1, LOGO2, NULL};
static const char	*g_r4[] = {LOGO0, LOGO1, LOGO2, LOGO3, NULL};
static const char	*g_r5[] = {LOGO0, LOGO1, LOGO2, LOGO3, LOGO4, NULL};
static const char	*g_r6[] = {LOGO0, LOGO1, LOGO2, LOGO3, LOGO4, LOGO5, NULL};
static const char	*g_full[] = {
	LOGO0, LOGO1, LOGO2, LOGO3, LOGO4, LOGO5, LOGO6, NULL};

/* The frame table: the logo drawn in row by row, then held. `count` receives
   the number of frames. */
const char	***intro_frames(size_t *count)
{
	static const char	**frames[] = {
		g_r1, g_r2, g_r3, g_r4, g_r5, g_r6, g_full, g_full, g_full};

	if (count)
		*count = sizeof(frames) / sizeof(frames[0]);
	return (frames);
}
