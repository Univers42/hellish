/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtin_shopt2.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/21 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/07/21 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"

/* Split from builtin_shopt.c only because the norm caps a file at 5
   functions. */

/* Collect the flags. Every character of every leading -word is read, not
   just the first: -q is a MODIFIER that combines freely with the action
   flags, so `shopt -qs extglob` means "set it, quietly" and used to be
   parsed as a bare -q (a query) because only argv[i][1] was inspected.
   That made it report failure on an option it had just switched on. */
size_t	shopt_flags(t_vec argv, char *act, int *quiet)
{
	size_t	i;
	size_t	j;

	i = 1;
	while (i < argv.len && ((char **)argv.ctx)[i][0] == '-'
		&& ((char **)argv.ctx)[i][1])
	{
		j = 1;
		while (((char **)argv.ctx)[i][j])
		{
			if (((char **)argv.ctx)[i][j] == 'q')
				*quiet = 1;
			else if (ft_strchr("supo", ((char **)argv.ctx)[i][j]))
				*act = ((char **)argv.ctx)[i][j];
			j++;
		}
		i++;
	}
	return (i);
}
