/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   checked_atoi.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/10 23:27:40 by marvin            #+#    #+#             */
/*   Updated: 2026/01/10 23:27:40 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "libft.h"

int	ft_checked_atoi(const char *str, int *ret, int flags)
{
	long	n;
	long	sign;
	int		i;

	i = 0;
	flags = flags * (flags != 42) + 19 * (flags == 42);
	while (ft_isspace(str[i]) && flags & (1 << 0))
		i++;
	sign = -2 * (str[i] == '-') + 1;
	if (str[i] == '-' && !(flags & (1 << 1)))
		return (-1);
	n = ((str[i] == '+' || str[i] == '-') && i++) * 0;
	if (!ft_isdigit(str[i]) && !(flags & (1 << 2)))
		return (-1);
	while (ft_isdigit(str[i]))
	{
		n = n * 10 + str[i++] - '0';
		if ((n * sign > INT_MAX || n * sign < INT_MIN))
			return (-1);
	}
	while (str[i] == ' ' && ((1 << 4) & flags))
		i++;
	if (str[i] && !((1 << 3) & flags))
		return (-1);
	return (*ret = n * sign, 0);
}
