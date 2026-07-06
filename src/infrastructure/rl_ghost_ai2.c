/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   rl_ghost_ai2.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/22 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/22 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "rl_ghost_ai.h"
#include "rl_private.h"
#include "ft_builtins.h"
#include <unistd.h>
#include <stdlib.h>

/* $PATH split once, cached for the life of the readline child. */
static char	**path_dirs(void)
{
	static char	**dirs;
	char		*p;

	if (!dirs)
	{
		p = getenv("PATH");
		if (p)
			dirs = ft_split(p, ':');
	}
	return (dirs);
}

/* 1 if `word` names an executable (a slash path that is X_OK, or found in any
   $PATH dir). */
static int	in_path(const char *word)
{
	char	full[1024];
	char	**dirs;
	int		i;

	if (ft_strchr(word, '/'))
		return (access(word, X_OK) == 0);
	dirs = path_dirs();
	i = 0;
	while (dirs && dirs[i])
	{
		ft_strlcpy(full, dirs[i], sizeof(full));
		ft_strlcat(full, "/", sizeof(full));
		ft_strlcat(full, word, sizeof(full));
		if (access(full, X_OK) == 0)
			return (1);
		i++;
	}
	return (0);
}

/* 1 if the first word of `s` is a runnable command (a builtin or in $PATH), so
   we never ghost a typo like `cleasr` back at the user. Runs per candidate per
   redisplay, and consecutive candidates usually share a first word, so the
   last verdict is memoized -- one $PATH access() walk instead of dozens. */
int	cmd_resolvable(const char *s)
{
	static char	memo[256];
	static int	verdict;
	char		word[256];
	int			i;

	i = 0;
	while (s[i] && s[i] != ' ' && i < 255)
	{
		word[i] = s[i];
		i++;
	}
	word[i] = '\0';
	if (!word[0])
		return (0);
	if (memo[0] && !ft_strcmp(word, memo))
		return (verdict);
	ft_strlcpy(memo, word, sizeof(memo));
	verdict = (builtin_func(word) != NULL || in_path(word));
	return (verdict);
}

/* The AI suggestion's suffix, if one has landed and still extends `line`
   (borrowed; do not free). NULL otherwise. */
const char	*ai_ghost_get(const char *line)
{
	t_aig	*a;
	size_t	len;

	a = aig();
	if (!a->sug[0])
		return (NULL);
	len = ft_strlen(line);
	if (!ft_strncmp(a->sug, line, len) && ft_strlen(a->sug) > len)
		return (a->sug + len);
	return (NULL);
}

/* readline idle hook (~100ms): service resizes, paint the dim ghost once the
   line settles, and drive the async AI suggestion without ever blocking. On a
   line change, cancel + remember it (the getc wrapper already wiped the old
   paint); while a worker runs, poll and repaint when it lands; once the line
   settles (and history has nothing), fire one worker. History ghost-text takes
   priority, so AI only fills the gaps. The live AI fetch is OPT-IN
   ($HELLISH_AI_SUGGEST): on a CPU backend its inference bursts fight your real
   commands; enable it on a GPU or fast cloud, where it is near-instant. */
int	rl_ai_event(void)
{
	t_aig	*a;

	if (rl_resize_fixup())
	{
		ghost_erase_pending();
		rl_forced_update_display();
	}
	a = aig();
	if (ft_strncmp(a->line, rl_line_buffer, sizeof(a->line)) != 0)
	{
		aig_reset(a);
		ft_strlcpy(a->line, rl_line_buffer, sizeof(a->line));
		return (0);
	}
	if (a->fd >= 0)
	{
		aig_poll(a);
		if (a->sug[0])
			ghost_erase_pending();
	}
	else if (getenv("HELLISH_AI_SUGGEST") && !a->fired && rl_line_buffer[0]
		&& ft_strlen(rl_line_buffer) >= 2 && !ai_history_has(rl_line_buffer))
		aig_fire(a, rl_line_buffer);
	return (ghost_draw(), 0);
}
