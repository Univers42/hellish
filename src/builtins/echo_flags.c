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

#include "builtins_private.h"

static int	is_flag_char(char c)
{
	return (ft_strchr("nEe", c) != NULL);
}

static void	apply_flag_char(char c, int *tn, int *te)
{
	if (c == 'n')
		*tn = 1;
	else if (c == 'e')
		*te = 1;
	else if (c == 'E')
		*te = 0;
}

static int	process_flag_token(char *token, int *tn, int *te)
{
	size_t	j;
	int		local_tn;
	int		local_te;
	int		valid;

	local_tn = *tn;
	local_te = *te;
	j = 1;
	valid = 1;
	while (token[j])
	{
		if (!is_flag_char(token[j]))
		{
			valid = 0;
			break ;
		}
		apply_flag_char(token[j], &local_tn, &local_te);
		j++;
	}
	if (!valid)
		return (0);
	*tn = local_tn;
	*te = local_te;
	return (1);
}

int	parse_flags(t_vec argv, int *n, int *e)
{
	size_t	i;

	i = 0;
	while (++i < argv.len && ((char **)argv.ctx)[i][0] == '-' &&
		((char **)argv.ctx)[i][1])
	{
		if (!process_flag_token(((char **)argv.ctx)[i], n, e))
			break ;
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
