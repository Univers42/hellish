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
   listing the permissions that ARE allowed (the complement of the mask).
   With -p as well, bash prefixes the reusable form: "umask -S u=...". */
int	umask_symbolic(int flags)
{
	mode_t	m;
	int		allowed;

	m = umask(0);
	umask(m);
	allowed = (~m) & 0777;
	if (flags & 2)
		ft_printf("umask -S ");
	umask_class('u', (allowed >> 6) & 7);
	ft_printf(",");
	umask_class('g', (allowed >> 3) & 7);
	ft_printf(",");
	umask_class('o', allowed & 7);
	return (ft_printf("\n"), 0);
}

/* Cluster options the way bash's internal getopt does: -S (bit 1) and -p
   (bit 2) may repeat or combine, "--" ends options, and a lone "-" is an
   operand (it is a valid, empty symbolic mode).  Returns the flag bits
   with *idx on the first operand, or -1 on an unknown option (status 2
   at the call site, matching bash --posix).  The argv vector is NOT
   NULL-terminated, so `len` bounds the scan. */
int	umask_opts(char **av, size_t len, int *idx)
{
	int	flags;
	int	j;

	flags = 0;
	while ((size_t)*idx < len && av[*idx][0] == '-' && av[*idx][1])
	{
		if (ft_strcmp(av[*idx], "--") == 0)
			return ((*idx)++, flags);
		j = 1;
		while (av[*idx][j])
		{
			if (av[*idx][j] == 'S')
				flags |= 1;
			else if (av[*idx][j] == 'p')
				flags |= 2;
			else
				return (-1);
			j++;
		}
		(*idx)++;
	}
	return (flags);
}

/* No-operand display: -S wins (symbolic, -p aware), plain -p prints the
   eval-able "umask 0022", and the default is the bare 4-digit octal. */
int	umask_report(mode_t m, int flags)
{
	if (flags & 1)
		return (umask_symbolic(flags));
	if (flags & 2)
		ft_printf("umask ");
	return (ft_printf("0%c%c%c\n", '0' + ((m >> 6) & 7),
			'0' + ((m >> 3) & 7), '0' + (m & 7)), 0);
}
