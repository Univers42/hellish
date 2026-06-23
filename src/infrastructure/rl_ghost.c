/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   rl_ghost.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/22 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/22 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "rl_private.h"
#include "rl_ghost_ai.h"
#include <readline/history.h>

/* 1 if some history entry begins with `line` AND is a real, runnable command
   -- so a settled line with only a typo in history still triggers the smart AI
   suggestion instead of ghosting the typo back. */
int	ai_history_has(const char *line)
{
	HIST_ENTRY	**h;
	int			i;
	size_t		len;

	h = history_list();
	if (!h || !line[0])
		return (0);
	len = ft_strlen(line);
	i = history_length - 1;
	while (i >= 0)
	{
		if (h[i] && !ft_strncmp(h[i]->line, line, len)
			&& ft_strlen(h[i]->line) > len && ft_strlen(h[i]->line) <= 256
			&& cmd_resolvable(h[i]->line))
			return (1);
		i--;
	}
	return (0);
}

/* The dim suggestion's suffix. A ready AI suggestion (smart) wins; else the
   most recent matching history entry whose command resolves -- never a typo
   like `cleasr`. Borrowed; do not free. NULL at non-end-of-line. */
static const char	*ghost_suffix(void)
{
	HIST_ENTRY	**h;
	const char	*ai;
	int			i;
	size_t		len;

	if (rl_point != rl_end || !rl_line_buffer || !*rl_line_buffer)
		return (NULL);
	ai = ai_ghost_get(rl_line_buffer);
	if (ai)
		return (ai);
	h = history_list();
	len = ft_strlen(rl_line_buffer);
	i = history_length - 1;
	while (h && i >= 0)
	{
		if (h[i] && !ft_strncmp(h[i]->line, rl_line_buffer, len)
			&& ft_strlen(h[i]->line) > len && ft_strlen(h[i]->line) <= 256
			&& cmd_resolvable(h[i]->line))
			return (h[i]->line + len);
		i--;
	}
	return (NULL);
}

/* Custom redisplay: draw the real line, erase any stale suggestion to end of
   line, then draw the current dim suffix and step the cursor back. The erase
   (\033[K) every frame is what stops old suggestions from piling up as you
   type. Only acts at end-of-line, so it never eats typed text; truncates the
   suffix to what fits on the row so a long history entry can't wrap and break
   the cursor math. ponytail: ASCII columns (rl_point past the 4-col prompt). */
void	ghost_redisplay(void)
{
	const char	*g;
	int			rows;
	int			cols;
	int			n;

	rl_redisplay();
	if (rl_point != rl_end)
		return ;
	fprintf(rl_outstream, "\033[K");
	g = ghost_suffix();
	if (g && *g)
	{
		rl_get_screen_size(&rows, &cols);
		n = (int)ft_strlen(g);
		if (n > cols - rl_point - 5)
			n = cols - rl_point - 5;
		if (n > 0)
			fprintf(rl_outstream, "\033[90m%.*s\033[0m\033[%dD", n, g, n);
	}
	fflush(rl_outstream);
}

/* Right-arrow: accept the whole suggestion when one is showing, otherwise the
   normal forward-char. */
int	rl_ghost_accept(int count, int key)
{
	const char	*g;

	g = ghost_suffix();
	if (g && *g)
		return (rl_insert_text((char *)g), 0);
	return (rl_forward_char(count, key));
}
