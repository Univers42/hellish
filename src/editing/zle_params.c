/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   zle_params.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/17 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/09/17 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "shell.h"
#include "libft.h"
#include "env.h"
#include "zle.h"
#include "ft_builtins.h"

/* BUFFER, LBUFFER, RBUFFER and CURSOR exist only while a widget runs.
**
** That is zsh's rule, and the child the line used to be read in enforced
** it for free: the child died with its variables. In the shell process
** they would stay behind after every widget -- a script testing
** ${BUFFER-unset} would see the last edited line -- so a widget's view of
** them is scoped: what was there before is put back, or the name unset. */

static const char	*zle_param_name(int i)
{
	static const char	*names[] = {"BUFFER", "LBUFFER", "RBUFFER",
		"CURSOR"};

	return (names[i]);
}

void	zle_params_save(t_shell *state, t_zle_saved *s)
{
	int		i;
	t_env	*e;

	i = -1;
	while (++i < ZLE_NPARAMS)
	{
		s->val[i] = NULL;
		e = env_get(&state->env, (char *)zle_param_name(i));
		if (e && e->value)
			s->val[i] = ft_strdup(e->value);
	}
}

void	zle_params_restore(t_shell *state, t_zle_saved *s)
{
	int	i;

	i = -1;
	while (++i < ZLE_NPARAMS)
	{
		if (s->val[i])
			env_set(&state->env, env_create(
					ft_strdup((char *)zle_param_name(i)), s->val[i], false));
		else
			try_unset(state, (char *)zle_param_name(i));
		s->val[i] = NULL;
	}
}
