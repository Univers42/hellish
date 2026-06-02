/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtin_type2.c                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/02 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/02 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"
#include "sh_alias.h"
#include "cmd_hash.h"

int	builtin_type(t_shell *state, t_vec argv)
{
	size_t	i;
	int		status;

	if (argv.len < 2)
		return (0);
	status = 0;
	i = 1;
	while (i < argv.len)
	{
		if (type_one(state, ((char **)argv.ctx)[i]))
			status = 1;
		i++;
	}
	return (status);
}
