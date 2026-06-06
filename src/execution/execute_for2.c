/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   execute_for2.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/03/14 12:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/03/14 12:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "execution_private.h"

/* Assign the loop variable for one for-loop iteration.  If the variable
   already exists we update it in place (swap the value string, reuse the
   env entry) to avoid creating duplicate entries.  If it is new we create
   a non-exported entry; it will become exported only if allexport is set
   via env_set (which calls env_check_export). */
void	set_for_var(t_shell *state, char *name, char *val)
{
	t_env	*old;

	old = env_get(&state->env, name);
	if (old)
	{
		xfree(old->value);
		old->value = ft_strdup(val);
		return ;
	}
	env_set(&state->env,
		env_create(ft_strdup(name), ft_strdup(val), false));
}
