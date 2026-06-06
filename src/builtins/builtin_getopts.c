/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtin_getopts.c                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/02 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/02 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"

/* Set a one-character shell variable — used throughout getopts to update
   the option-name variable and OPTARG. The buf is a two-byte local so we
   never need heap memory for a single character. */
void	gopt_set_char(t_shell *state, const char *name, char c)
{
	char	buf[2];

	buf[0] = c;
	buf[1] = '\0';
	env_set(&state->env, env_create(ft_strdup(name), ft_strdup(buf), false));
}

/* Return the idx-th (1-based) argument being parsed. When the user called
   `getopts optstr name arg1 arg2 …` the extra args live in argv[3..]; when
   called without extra args (the common case) we fall back to the positional
   parameters $1, $2, … from the environment. */
char	*gopt_arg(t_shell *state, t_vec argv, int idx)
{
	char	*k;
	char	*v;

	if (argv.len > 3)
	{
		if (idx < 1 || (size_t)(idx + 2) >= argv.len)
			return (NULL);
		return (((char **)argv.ctx)[idx + 2]);
	}
	k = ft_itoa(idx);
	v = env_expand(state, k);
	return (xfree(k), v);
}

/* Persist OPTIND back to the environment so the next call picks up where
   this one left off. getopts is designed to be called in a loop, so the
   state between calls lives in $OPTIND rather than in the shell struct. */
void	gopt_commit_optind(t_shell *state, int optind)
{
	env_set(&state->env, env_create(ft_strdup("OPTIND"), ft_itoa(optind),
			false));
}

/* An option required an argument but none was available. In silent mode
   (optstring starts with ':'), set the name var to ':' and OPTARG to the
   bad option letter — the script handles the error itself. Otherwise print
   the error message and set the name var to '?'. Both paths return 0 (no
   more options) so the loop can continue with the next word. */
static int	gopt_want_error(t_shell *state, t_getopts *g, char *cur, int pos)
{
	state->getopts_pos = 0;
	g->optind++;
	if (g->silent)
	{
		gopt_set_char(state, g->name, ':');
		gopt_set_char(state, "OPTARG", cur[pos]);
		return (0);
	}
	ft_eprintf("%s: option requires an argument -- %c\n",
		state->dft_ctx, cur[pos]);
	return (gopt_set_char(state, g->name, '?'), 0);
}

/* Option needs an argument: take it attached or from the next word. */
int	gopt_want_arg(t_shell *state, t_vec argv, t_getopts *g, char *cur)
{
	int	pos;

	pos = state->getopts_pos;
	if (cur[pos + 1])
	{
		env_set(&state->env, env_create(ft_strdup("OPTARG"),
				ft_strdup(cur + pos + 1), false));
		state->getopts_pos = 0;
		g->optind++;
		return (1);
	}
	if (g->optind + 1 <= g->count)
	{
		env_set(&state->env, env_create(ft_strdup("OPTARG"),
				ft_strdup(gopt_arg(state, argv, g->optind + 1)), false));
		state->getopts_pos = 0;
		g->optind += 2;
		return (1);
	}
	return (gopt_want_error(state, g, cur, pos));
}
