/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ai_config.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/22 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/22 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ai.h"
#include "env.h"
#include "sh_input.h"
#include <stdlib.h>

/* hellish keeps its own variable store (state->env); getenv() reads the C
   environ, and `export` does not sync the two. So copy the HELLISH_AI_* vars
   into environ here, so getenv() (used throughout the AI client, including the
   forked readline child that only inherits environ) sees a config set via
   ~/.hellishrc or `export`. Called at startup and before each AI request. */
void	ai_sync_env(t_shell *state)
{
	char	*keys[7];
	char	*v;
	int		i;

	keys[0] = "HELLISH_AI_URL";
	keys[1] = "HELLISH_AI_KEY";
	keys[2] = "HELLISH_AI_MODEL";
	keys[3] = "HELLISH_AI_HOST";
	keys[4] = "HELLISH_AI_PORT";
	keys[5] = "HELLISH_AI_TIMEOUT_MS";
	keys[6] = "HELLISH_AI_SUGGEST";
	i = 0;
	while (i < 7)
	{
		v = env_expand(state, keys[i]);
		if (v && *v)
			setenv(keys[i], v, 1);
		else
			unsetenv(keys[i]);
		i++;
	}
}

/* Per interactive REPL turn: mirror the AI env vars into environ (so the forked
   readline child sees HELLISH_AI_*), export the last exit status for the AI
   context (failure is the strongest predictor of the next command), then
   refresh the background pro-tip. Kept out of the hot non-interactive path. */
void	ai_prompt_prep(t_shell *state)
{
	char	*st;

	if (state->metinp == INP_RL)
	{
		ai_sync_env(state);
		st = ft_itoa(state->last_cmd_st_exe.status);
		if (st)
			setenv("HELLISH_LAST_STATUS", st, 1);
		xfree(st);
	}
	ai_tip_spawn(state);
}

/* Server host: $HELLISH_AI_HOST or the loopback default. The pointer is
   borrowed (environ or a literal) -- callers must NOT free it.
   ponytail: numeric IP only (ai_connect uses inet_pton); the docker port is
   published on 127.0.0.1, so a hostname is never needed here. */
char	*ai_host(void)
{
	char	*h;

	h = getenv("HELLISH_AI_HOST");
	if (h && *h)
		return (h);
	return ("127.0.0.1");
}

int	ai_port(void)
{
	char	*p;

	p = getenv("HELLISH_AI_PORT");
	if (p && *p)
		return (ft_atoi(p));
	return (8080);
}

/* Round-trip budget for a completion. LLMs are slow, so the default is
   generous; the passive hooks (tips, ghost-text) pass their own tight value. */
int	ai_timeout_ms(void)
{
	char	*t;

	t = getenv("HELLISH_AI_TIMEOUT_MS");
	if (t && *t)
		return (ft_atoi(t));
	return (20000);
}
