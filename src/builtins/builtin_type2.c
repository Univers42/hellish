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

/* type name ...: classify each name. Exit status 0 only when all names were
   found; 1 if any one was not. Errors do not abort — we keep going and let
   the accumulated status speak for itself at the end. */
int	builtin_type(t_shell *state, t_vec argv)
{
	size_t	i;
	int		status;
	char	mode;

	mode = 0;
	i = 1;
	while (i < argv.len && ((char **)argv.ctx)[i][0] == '-'
		&& ((char **)argv.ctx)[i][1])
	{
		if (ft_strchr(((char **)argv.ctx)[i], 't'))
			mode = 't';
		else if (ft_strchr(((char **)argv.ctx)[i], 'P'))
			mode = 'P';
		else if (ft_strchr(((char **)argv.ctx)[i], 'p'))
			mode = 'p';
		i++;
	}
	status = 0;
	while (i < argv.len)
	{
		if (type_dispatch(state, ((char **)argv.ctx)[i], mode))
			status = 1;
		i++;
	}
	return (status);
}
