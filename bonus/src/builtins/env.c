/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   env.c                                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 23:29:59 by marvin            #+#    #+#             */
/*   Updated: 2026/01/09 23:29:59 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins.h"

int	builtin_env(t_state *state, t_vec argv)
{
	size_t	i;

	i = 0;
	(void)argv;
	while (i < state->env.len)
	{
		t_env	*e = &((t_env *)state->env.ctx)[i];

		if (e->exported)
			ft_printf("%s=%s\n", e->key, e->value);
		++i;
	}
	return (0);
}
