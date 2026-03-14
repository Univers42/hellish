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

int	builtin_set(t_shell *state, t_vec argv)
{
	size_t	i;
	t_env	*e;

	(void)argv;
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
