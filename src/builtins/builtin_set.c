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
