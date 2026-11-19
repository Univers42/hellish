/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   rl_dumb.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/19 23:30:00 by marvin            #+#    #+#             */
/*   Updated: 2026/11/19 23:30:00 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "rl_private.h"
#include "env.h"

/* Readline 8.1 can overrun line margins in horizontal-scroll mode when
   invisible prompt markers are present. Dumb terminals use that mode. */
void	rl_fix_dumb_prompt(t_shell *state, char *s)
{
	t_env	*e;
	size_t	r;
	size_t	w;

	if (!state || !s)
		return ;
	e = env_get(&state->env, "TERM");
	if (!e || !e->value || ft_strcmp(e->value, "dumb"))
		return ;
	r = 0;
	w = 0;
	while (s[r])
	{
		if (s[r] != '\001' && s[r] != '\002')
			s[w++] = s[r];
		r++;
	}
	s[w] = '\0';
}
