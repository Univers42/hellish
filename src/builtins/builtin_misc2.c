/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtin_misc2.c                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/03 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/03 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"
#include <sys/stat.h>

/* Print one permission class (u/g/o) of `umask -S`: the bits NOT masked,
   as the rwx letters that are permitted. */
static void	umask_class(char who, int bits)
{
	ft_printf("%c=", who);
	if (bits & 4)
		ft_printf("r");
	if (bits & 2)
		ft_printf("w");
	if (bits & 1)
		ft_printf("x");
}

/* umask -S : print the file-creation mask symbolically (u=...,g=...,o=...),
   listing the permissions that ARE allowed (the complement of the mask). */
int	umask_symbolic(void)
{
	mode_t	m;
	int		allowed;

	m = umask(0);
	umask(m);
	allowed = (~m) & 0777;
	umask_class('u', (allowed >> 6) & 7);
	ft_printf(",");
	umask_class('g', (allowed >> 3) & 7);
	ft_printf(",");
	umask_class('o', allowed & 7);
	return (ft_printf("\n"), 0);
}
