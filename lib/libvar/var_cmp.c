/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   var_cmp.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: alcacere <alcacere@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/04 19:35:30 by alcacere          #+#    #+#             */
/*   Updated: 2025/11/04 20:02:57 by alcacere         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

/*
 * Compares two strings up to the first = or '\0'.  The first
 * string must be terminated by '='; the second may be terminated by
 * either '=' or '\0'.
 */
int	varcmp(const char *p, const char *q)
{
	int	c;
	int	d;

	c = *p;
	d = *q;
	while (c == d)
	{
		if (!c)
			break ;
		c = *p++;
		d = *q++;
		if (c == '=')
			c = '\0';
		if (d == '=')
			d = '\0';
	}
	return (c - d);
}
