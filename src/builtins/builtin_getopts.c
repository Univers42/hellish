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

typedef struct s_getopts
{
	char	*optstring;
	char	*name;
	int		optind;
	int		count;
	bool	silent;
}	t_getopts;

static void	set_var(t_shell *state, const char *name, const char *val)
{
	env_set(&state->env, env_create(ft_strdup(name), ft_strdup(val), false));
}

static void	set_char(t_shell *state, const char *name, char c)
{
	char	buf[2];

	buf[0] = c;
	buf[1] = '\0';
	set_var(state, name, buf);
}

/* The idx-th (1-based) argument: explicit args after `name`, else positionals. */
static char	*gopt_arg(t_shell *state, t_vec argv, int idx)
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
	return (free(k), v);
}

static void	commit_optind(t_shell *state, int optind)
{
	env_set(&state->env, env_create(ft_strdup("OPTIND"), ft_itoa(optind),
			false));
}

/* Option needs an argument: take it attached (-xVAL) or from the next word. */
static int	want_arg(t_shell *state, t_vec argv, t_getopts *g, char *cur)
{
	int	pos;

	pos = state->getopts_pos;
	if (cur[pos + 1])
		return (set_var(state, "OPTARG", cur + pos + 1),
			state->getopts_pos = 0, ++g->optind, 1);
	if (g->optind + 1 <= g->count)
		return (set_var(state, "OPTARG", gopt_arg(state, argv, g->optind + 1)),
			state->getopts_pos = 0, g->optind += 2, 1);
	state->getopts_pos = 0;
	g->optind++;
	if (g->silent)
		return (set_char(state, g->name, ':'), set_char(state, "OPTARG",
				cur[pos]), 0);
	ft_eprintf("%s: option requires an argument -- %c\n",
		state->dft_ctx, cur[pos]);
	return (set_char(state, g->name, '?'), 0);
}

static int	one_option(t_shell *state, t_vec argv, t_getopts *g, char *cur)
{
	int		pos;
	char	*spec;

	pos = state->getopts_pos;
	spec = ft_strchr(g->optstring, cur[pos]);
	if (!spec || cur[pos] == ':')
	{
		if (g->silent)
			set_char(state, "OPTARG", cur[pos]);
		else
			ft_eprintf("%s: illegal option -- %c\n",
				state->dft_ctx, cur[pos]);
		set_char(state, g->name, '?');
		if (!cur[pos + 1])
			(g->optind++, state->getopts_pos = 0);
		else
			state->getopts_pos = pos + 1;
		return (commit_optind(state, g->optind), 0);
	}
	if (spec[1] == ':')
		return (want_arg(state, argv, g, cur), set_char(state, g->name,
				cur[pos]), commit_optind(state, g->optind), 0);
	set_char(state, g->name, cur[pos]);
	if (!cur[pos + 1])
		(g->optind++, state->getopts_pos = 0);
	else
		state->getopts_pos = pos + 1;
	return (commit_optind(state, g->optind), 0);
}

int	builtin_getopts(t_shell *state, t_vec argv)
{
	t_getopts	g;
	char		*oi;
	char		*cur;

	if (argv.len < 3)
		return (ft_eprintf("getopts: usage: getopts optstring name [arg]\n"), 2);
	g.optstring = ((char **)argv.ctx)[1];
	g.name = ((char **)argv.ctx)[2];
	g.silent = (g.optstring[0] == ':');
	oi = env_expand(state, "OPTIND");
	g.optind = ((oi) ? ft_atoi(oi) : 1);
	if (g.optind < 1)
		g.optind = 1;
	if (state->getopts_pos < 1)
		state->getopts_pos = 1;
	g.count = ((argv.len > 3) ? (int)argv.len - 3
			: ft_atoi(env_expand(state, "#") ? env_expand(state, "#") : "0"));
	cur = ((g.optind <= g.count) ? gopt_arg(state, argv, g.optind) : NULL);
	if (!cur || cur[0] != '-' || !cur[1] || (cur[1] == '-' && !cur[2]))
		return (set_char(state, g.name, '?'), state->getopts_pos = 0,
			commit_optind(state, g.optind + (cur && cur[1] == '-')), 1);
	if (!cur[state->getopts_pos])
		return (state->getopts_pos = 0, commit_optind(state, g.optind + 1),
			builtin_getopts(state, argv));
	return (one_option(state, argv, &g, cur));
}
