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

int	handle_set_o(t_shell *state, t_vec argv)
{
	char	**av;

	av = (char **)argv.ctx;
	if (argv.len < 3)
		return (list_set_options(state));
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
void	pos_set_cnt(t_pos *pos)
{
	char	*s;

	s = ft_itoa(pos->count);
	if (s)
		ft_strlcpy(pos->cnt_str, s, sizeof(pos->cnt_str));
	else
		ft_strlcpy(pos->cnt_str, "0", sizeof(pos->cnt_str));
	free(s);
}

/* Fill `pos` with fresh dups of args[0..n-1]. */
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
	pos->args_owned = true;
	pos_set_cnt(pos);
}

/* Release the positional array; string elements are freed only when owned (a
   borrowed set points into the caller's argv, which owns/frees the strings). */
void	pos_free(t_pos *pos)
{
	size_t	i;

	if (pos->args)
	{
		i = 0;
		if (pos->args_owned)
			while (pos->args[i])
				free(pos->args[i++]);
		free(pos->args);
	}
	pos->args = NULL;
	pos->count = 0;
	pos->args_owned = false;
}

int	set_positional_args(t_shell *state, char **args, size_t n)
{
	pos_free(&state->pos);
	pos_build(&state->pos, args, n);
	return (0);
}
