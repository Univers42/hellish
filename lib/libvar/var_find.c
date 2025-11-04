/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   var_find.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: alcacere <alcacere@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/04 19:35:34 by alcacere          #+#    #+#             */
/*   Updated: 2025/11/04 20:02:18 by alcacere         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "var.h"

t_var	**findvar(const char *name)
{
	t_var	**vpp;

	vpp = hashvar(name);
	while (*vpp)
	{
		if (varequal((*vpp)->text, name))
			break ;
		vpp = &(*vpp)->next;
	}
	return (vpp);
}
