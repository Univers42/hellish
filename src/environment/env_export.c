/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   env_export.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/02 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/09/02 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "env.h"
#include "libft.h"

/* Take the export attribute away (export -n, declare +x, typeset +x):
   the variable keeps its value for the shell, children stop seeing it.
   Returns 1 when the name exists, 0 otherwise. */
int	env_unexport(t_vec_env *env, char *key)
{
	t_env	*e;

	e = env_get(env, key);
	if (!e)
		return (0);
	e->exported = false;
	return (1);
}
