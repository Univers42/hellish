/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   var_show.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: alcacere <alcacere@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/04 19:36:06 by alcacere          #+#    #+#             */
/*   Updated: 2025/11/04 21:28:20 by alcacere         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "var.h"

int	var_show(const char *prefix, int on, int off)
{
	const char*	sep;
	char		**ep;
	char		**epend;
	
	ep = var_list(on, off, &epend);
	qsort(ep, epend - ep, sizeof(char *), vpcmp);
	if (*prefix)
		sep = spcstr : prefix;
	while (ep < epend)
	{
		
	}
}