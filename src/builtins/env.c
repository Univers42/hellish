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
#include "env.h"

int	builtin_env(t_shell *state, t_vec argv)
{
	size_t	i;
	t_env	*e;

	i = -1;
	(void)argv;
	while (++i < state->env.len)
	{
		e = &((t_env *)state->env.ctx)[i];
		if (e->exported)
			ft_printf("%s=%s\n", e->key, e->value);
	}
	return (0);
}
