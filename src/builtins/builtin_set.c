/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtin_set.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/03/14 09:30:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/03/14 09:30:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"

static int	handle_set_o(t_shell *state, t_vec argv)
{
	char	**av;

	av = (char **)argv.ctx;
	if (argv.len < 3)
	{
		if (state->edit_mode == 0)
			ft_printf("vi\ton\nemacs\toff\n");
		else
			ft_printf("vi\toff\nemacs\ton\n");
		return (0);
	}
	if (ft_strcmp(av[2], "vi") == 0)
	{
		state->edit_mode = 0;
		state->rl.edit_mode = 0;
		return (0);
	}
	if (ft_strcmp(av[2], "emacs") == 0)
	{
		state->edit_mode = 1;
		state->rl.edit_mode = 1;
		return (0);
	}
	ft_eprintf("%s: set: %s: invalid option name\n", state->ctx, av[2]);
	return (1);
}

/*
** set [--] [arg ...]: replace the positional parameters ($1.., $#).
** Stored as env entries "1".."N" + "#" (same scheme function calls use).
*/
static int	set_positional_args(t_shell *state, char **args, size_t n)
{
	char	*cnt;
	size_t	i;
	char	*key;

	cnt = env_expand(state, "#");
	i = (size_t)ft_atoi(cnt ? cnt : "0");
	while (i >= 1)
	{
		key = ft_itoa((int)i--);
		try_unset(state, key);
		free(key);
	}
	i = 0;
	while (i < n)
	{
		key = ft_itoa((int)(i + 1));
		env_set(&state->env, env_create(key, ft_strdup(args[i]), false));
		i++;
	}
	env_set(&state->env, env_create(ft_strdup("#"), ft_itoa((int)n), false));
	return (0);
}

int	builtin_set(t_shell *state, t_vec argv)
{
	size_t	i;
	t_env	*e;
	char	**av;

	av = (char **)argv.ctx;
	if (argv.len >= 2 && ft_strcmp(av[1], "-o") == 0)
		return (handle_set_o(state, argv));
	if (argv.len >= 2 && ft_strcmp(av[1], "+o") == 0)
		return (handle_set_o(state, argv));
	if (argv.len >= 2 && ft_strcmp(av[1], "--") == 0)
		return (set_positional_args(state, av + 2, argv.len - 2));
	if (argv.len >= 2 && av[1][0] != '-' && av[1][0] != '+')
		return (set_positional_args(state, av + 1, argv.len - 1));
	if (argv.len >= 2)
		return (0);
	i = 0;
	while (i < state->env.len)
	{
		e = &((t_env *)state->env.ctx)[i];
		if (e->key)
		{
			if (e->value)
				ft_printf("%s=%s\n", e->key, e->value);
			else
				ft_printf("%s\n", e->key);
		}
		i++;
	}
	return (0);
}

/* Collect positional params [from..count] (1-based) into a fresh NULL-
   terminated array of dup'd strings (for shift). */
static char	**collect_tail_positionals(t_shell *state, int from, int count)
{
	char	**vals;
	char	*k;
	char	*v;
	int		i;

	vals = ft_calloc((size_t)(count - from + 2), sizeof(char *));
	if (!vals)
		return (NULL);
	i = 0;
	while (from + i <= count)
	{
		k = ft_itoa(from + i);
		v = env_expand(state, k);
		vals[i] = ft_strdup(v ? v : "");
		free(k);
		i++;
	}
	return (vals);
}

int	builtin_shift(t_shell *state, t_vec argv)
{
	char	**vals;
	char	*cnt;
	int		count;
	int		n;

	cnt = env_expand(state, "#");
	count = ((cnt) ? ft_atoi(cnt) : 0);
	n = 1;
	if (argv.len >= 2)
		n = ft_atoi(((char **)argv.ctx)[1]);
	if (n < 0 || n > count)
		return (1);
	vals = collect_tail_positionals(state, n + 1, count);
	if (!vals)
		return (1);
	set_positional_args(state, vals, (size_t)(count - n));
	n = -1;
	while (vals[++n])
		free(vals[n]);
	return (free(vals), 0);
}
