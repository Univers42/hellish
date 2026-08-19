/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   update_cache.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/03 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/03 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "update.h"
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>

/* The latest version the last background check found, for callers that only
   want the string (the banner). 0 when nothing has been checked yet. */
int	read_cached_latest(char *out, size_t n)
{
	t_upd_state	s;

	if (!update_state_load(&s))
		return (0);
	ft_strlcpy(out, s.latest, n);
	return (out[0] != '\0');
}

/* How long ago the last successful check ran, in seconds; -1 if never. */
long	update_last_check_age(void)
{
	t_upd_state	s;

	update_state_load(&s);
	if (s.checked <= 0)
		return (-1);
	return ((long)time(NULL) - s.checked);
}

/* True when the last check is recent enough to skip a new one. A missing
   record is "stale", so the very first interactive run starts a check.
   HELLISH_UPDATE_TTL overrides the interval (seconds) -- the test suite
   uses it to force a re-check without waiting a day. */
static int	cache_is_fresh(void)
{
	long		age;
	const char	*ttl;
	long		limit;

	age = update_last_check_age();
	if (age < 0)
		return (0);
	limit = 86400;
	ttl = getenv("HELLISH_UPDATE_TTL");
	if (ttl && *ttl)
		limit = ft_atoi(ttl);
	return (age <= limit);
}

/* Interactive only: if the last check is stale, fork a fully detached child
   (double-fork) to refresh it in the background. Returns at once; the prompt
   is never delayed by the network, which is the hard requirement in issue
   #20 -- a slow or dead update server must cost the shell nothing at all.
   Opt out entirely with HELLISH_NO_UPDATE_CHECK. */
void	maybe_spawn_update_check(t_shell *state)
{
	pid_t	pid;

	if (state->metinp != INP_RL || !isatty(STDOUT_FILENO))
		return ;
	if (getenv("HELLISH_NO_UPDATE_CHECK") || cache_is_fresh())
		return ;
	pid = fork();
	if (pid != 0)
	{
		if (pid > 0)
			waitpid(pid, NULL, 0);
		return ;
	}
	if (fork() != 0)
		_exit(0);
	run_bg_update_check();
	_exit(0);
}
