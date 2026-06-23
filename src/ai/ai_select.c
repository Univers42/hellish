/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ai_select.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/22 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/22 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ai_select.h"
#include <unistd.h>

/* ponytail: mirrors raw_on in update_menu.c; kept separate because the menu's
   read_key collapses letters to nav keys, which a type-to-filter box can't. */
int	ai_raw_on(struct termios *saved)
{
	struct termios	raw;

	if (tcgetattr(STDIN_FILENO, saved) != 0)
		return (0);
	raw = *saved;
	raw.c_lflag &= ~(ICANON | ECHO);
	raw.c_cc[VMIN] = 1;
	raw.c_cc[VTIME] = 0;
	return (tcsetattr(STDIN_FILENO, TCSANOW, &raw) == 0);
}

/* Case-insensitive subsequence: every char of f appears in order in name. */
static int	sub_match(const char *name, const char *f)
{
	int	i;

	i = 0;
	if (!f[0])
		return (1);
	while (*name)
	{
		if (ft_tolower((unsigned char)*name) == ft_tolower((unsigned char)f[i]))
			i++;
		if (!f[i])
			return (1);
		name++;
	}
	return (!f[i]);
}

/* Recompute the matched-index list from the filter; clamp the selection. */
void	ai_refilter(t_aisel *s)
{
	int	i;

	s->m = 0;
	i = 0;
	while (i < s->n)
	{
		if (sub_match(s->all[i], s->filt))
			s->idx[s->m++] = i;
		i++;
	}
	if (s->sel >= s->m)
		s->sel = 0;
}

/* Move the selection with the up/down arrows, wrapping. */
static void	arrow(t_aisel *s, unsigned char c)
{
	if (s->m == 0)
		return ;
	if (c == 'A')
		s->sel = (s->sel + s->m - 1) % s->m;
	else if (c == 'B')
		s->sel = (s->sel + 1) % s->m;
}

/* Apply one key burst. Returns 1 (Enter), -1 (cancel) or 0 (keep going). */
int	ai_on_key(t_aisel *s, unsigned char *b, int r)
{
	if (r <= 0 || b[0] == 3 || (b[0] == 27 && r == 1))
		return (-1);
	if (b[0] == '\r' || b[0] == '\n')
		return (1);
	if (r >= 3 && b[0] == 27 && b[1] == '[')
		return (arrow(s, b[2]), 0);
	if ((b[0] == 127 || b[0] == 8) && s->flen > 0)
		return (s->filt[--s->flen] = '\0', ai_refilter(s), 0);
	if (ft_isprint(b[0]) && s->flen < 126)
	{
		s->filt[s->flen++] = (char)b[0];
		s->filt[s->flen] = '\0';
		ai_refilter(s);
	}
	return (0);
}
