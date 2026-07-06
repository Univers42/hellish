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

/* 1 if `s` is safe to ghost inline: extends the prefix (longer), short enough,
   SINGLE-LINE (a multi-line entry printed as a suffix emits raw newlines that
   desync the cursor and stack copies of the line -- the paste-corruption bug),
   and resolves to a real command so we never ghost a typo back. */
static int	ghostable(const char *s, size_t len)
{
	return (ft_strlen(s) > len && ft_strlen(s) <= 256
		&& !ft_strchr(s, '\n') && cmd_resolvable(s));
}

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
			&& ghostable(h[i]->line, len))
			return (1);
		i--;
	}
	return (0);
}

/* Whether ghost bytes are currently painted after the cursor. Set by
   ghost_draw, cleared by ghost_erase_pending (from the getc wrapper, so the
   screen is clean BEFORE readline processes any key and repaints). */
static int	g_ghost_on;

/* The dim suggestion's suffix. On an empty line, the next-command prediction
   (what usually follows the command that just ran). Otherwise a ready AI
   suggestion (smart) wins; else the most recent matching history entry whose
   command resolves -- never a typo like `cleasr`. Borrowed; do not free.
   NULL at non-end-of-line. ponytail: prefix ghost stays most-recent-match
   (fish/zsh behavior); add frequency weighting only if it ever feels wrong. */
const char	*ghost_suffix(void)
{
	HIST_ENTRY	**h;
	const char	*ai;
	int			i;
	size_t		len;

	if (rl_point != rl_end || !rl_line_buffer)
		return (NULL);
	if (!*rl_line_buffer)
		return (ghost_predict_empty());
	ai = ai_ghost_get(rl_line_buffer);
	if (ai)
		return (ai);
	h = history_list();
	len = ft_strlen(rl_line_buffer);
	i = history_length - 1;
	while (h && i >= 0)
	{
		if (h[i] && !ft_strncmp(h[i]->line, rl_line_buffer, len)
			&& ghostable(h[i]->line, len))
			return (h[i]->line + len);
		i--;
	}
	return (NULL);
}

/* Paint the dim suggestion after the cursor and step back, leaving readline's
   cursor untouched. Runs from the idle hook (never from a redisplay override:
   replacing rl_redisplay_function makes readline drop its multi-row rendering,
   so recalled multi-line history shows as `^J` soup). Truncated to the row and
   to the first newline so the cursor math can never break. Idempotent while
   already painted. 1 if a ghost is on screen. */
int	ghost_draw(void)
{
	const char	*g;
	const char	*nl;
	int			cols;
	int			n;

	if (g_ghost_on)
		return (1);
	if (rl_point != rl_end)
		return (0);
	g = ghost_suffix();
	if (!g || !*g)
		return (0);
	rl_get_screen_size(&n, &cols);
	n = (int)ft_strlen(g);
	nl = ft_strchr(g, '\n');
	if (nl)
		n = (int)(nl - g);
	if (n > cols - rl_point - 5)
		n = cols - rl_point - 5;
	if (n <= 0)
		return (0);
	fprintf(rl_outstream, "\033[90m%.*s\033[0m\033[%dD", n, g, n);
	fflush(rl_outstream);
	g_ghost_on = 1;
	return (1);
}

/* Erase a painted ghost (cursor-to-EOL wipes exactly the ghost bytes, since
   they sit after end-of-line). Called from the getc wrapper on every key, so
   readline always repaints over a clean row -- and an abandoned suggestion
   never survives into scrollback on Enter. 1 if something was erased. */
int	ghost_erase_pending(void)
{
	if (!g_ghost_on)
		return (0);
	g_ghost_on = 0;
	fputs("\033[K", rl_outstream);
	fflush(rl_outstream);
	return (1);
}
