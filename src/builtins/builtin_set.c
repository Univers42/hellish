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
	return (set_long_option(state, av[1][0], av[2]));
}

/* Cache the decimal of $# in the fixed buffer so env_expand can return it. */
static void	pos_set_cnt(t_pos *pos)
{
	char	*s;

	s = ft_itoa(pos->count);
	ft_strlcpy(pos->cnt_str, s ? s : "0", sizeof(pos->cnt_str));
	free(s);
}

/* Fill `pos` with fresh dups of args[0..n-1] WITHOUT freeing pos->args first
** (the caller has either freed or saved it). args[i] becomes $(i+1). */
void	pos_build(t_pos *pos, char **args, size_t n)
{
	size_t	i;

	pos->args = ft_calloc(n + 1, sizeof(char *));
	i = 0;
	while (pos->args && args && i < n)
	{
		pos->args[i] = ft_strdup(args[i]);
		i++;
	}
	pos->count = (int)n;
	pos_set_cnt(pos);
}

/* Release the strings + array owned by `pos`. */
void	pos_free(t_pos *pos)
{
	size_t	i;

	if (pos->args)
	{
		i = 0;
		while (pos->args[i])
			free(pos->args[i++]);
		free(pos->args);
	}
	pos->args = NULL;
	pos->count = 0;
}

/*
** set [--] [arg ...]: replace the positional parameters ($1.., $#).
** Stored in state->pos (outside the env) so function calls swap them cheaply.
*/
int	set_positional_args(t_shell *state, char **args, size_t n)
{
	pos_free(&state->pos);
	pos_build(&state->pos, args, n);
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
	if (argv.len >= 2 && (av[1][0] == '-' || av[1][0] == '+') && av[1][1])
		return (apply_set_flags(state, argv));
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
