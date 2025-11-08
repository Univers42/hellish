/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   restore_bash_trapsig.c                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/08 00:49:25 by dlesieur          #+#    #+#             */
/*   Updated: 2025/11/08 02:52:28 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "trap.h"

void	restore_bash_trapsig(char *oldval)
{
	if (oldval == 0)
		unbind_variable_noref("SH_TRAPSIG");
	else
	{
		bind_variable("SH_TRAPSIG", oldval, 0);
		free(oldval);
	}
}
