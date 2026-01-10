/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   echo_flags.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 23:30:17 by marvin            #+#    #+#             */
/*   Updated: 2026/01/09 23:30:17 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins.h"

int	parse_flags(t_vec argv, int *n, int *e)
{
	size_t	i;
	size_t	j;

	i = 1;
	while (i < argv.len && ((char **)argv.ctx)[i][0] == '-' && ((char **)argv.ctx)[i][1])
	{
		j = 1;
		while (((char **)argv.ctx)[i][j] && ft_strchr("nEe", ((char **)argv.ctx)[i][j]))
			j++;
		if (((char **)argv.ctx)[i][j])
			break ;
		j = 1;
		while (((char **)argv.ctx)[i][j])
		{
			if (((char **)argv.ctx)[i][j] == 'n')
				*n = 1;
			if (((char **)argv.ctx)[i][j] == 'e')
				*e = 1;
			if (((char **)argv.ctx)[i][j] == 'E')
				*e = 0;
			j++;
		}
		i++;
	}
	return (i);
}

int	print_args(int e, t_vec argv, size_t i)
{
	while (i < argv.len)
	{
		if (e)
		{
			if (e_parser(((char **)argv.ctx)[i]))
				return (1);
		}
		else
			ft_printf("%s", ((char **)argv.ctx)[i]);
		if (++i < argv.len)
			ft_printf(" ");
	}
	return (0);
}
