/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   var_init.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: alcacere <alcacere@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/04 19:35:41 by alcacere          #+#    #+#             */
/*   Updated: 2025/11/04 21:28:21 by alcacere         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "var.h"

void	var_init(void)
{
	t_var	*vp;
	t_var	*end;
	t_var	**vpp;

	vp = varinit;
	end = vp + sizeof(varinit) / sizeof(varinit[0]);
	while (++vp < end)
	{
		vpp = hashvar(vp->text);
		vp->next = *vpp;
		*vpp = vp;
	}
	if (!geteuid())
		vps1.text = "PS1=# ";
}
