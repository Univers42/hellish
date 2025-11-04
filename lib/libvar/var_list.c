/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   var_list.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: alcacere <alcacere@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/04 19:35:43 by alcacere          #+#    #+#             */
/*   Updated: 2025/11/04 21:26:49 by alcacere         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

/*
 * Generate a list of variables satisfying the given conditions.
 */
#include "var.h"
 
char	**listvars(int on, int off, char ***end)
{
	t_var	**vpp;
	t_var	*vp;
	char	**ep;
	int		mask;

	STARTSTACKSTR(ep);
	vpp = vartab;
	mask = on | off;
	while (vpp < vartab + VTABSIZE)
	{
		while (vp)
		{
			if ((vp->flags & mask) == on)
			{
				if (ep == stackstrend())
					ep = growstackstr();
				*ep++ = (char *) vp->text;
			}
			vp = vp->next;
		}
		vpp++;
	}
	if (ep == stackstrend())
		ep = growstackstr();
	if (end)
		*end = ep;
	return (*ep++ = NULL, grabstackstr(ep));
}
