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

/* Safe string-to-int with behaviour controlled by a flag bitmask. The magic
   value 42 remaps to 19 (0b10011) which is the "POSIX arithmetic context"
   preset: allow leading whitespace (bit 0), allow negative sign (bit 1),
   skip trailing garbage (bit 3). Other flags:
     bit 2 -- allow leading non-digits (treat them as 0)
     bit 4 -- strip trailing spaces
   Overflow in either direction returns -1 without touching *ret; on success
   stores the parsed value in *ret and returns 0. Sharing one function for
   every numeric-string parse in the shell kept the overflow handling in a
   single audited place -- critical for builtins like `ulimit` and `kill`. */
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
