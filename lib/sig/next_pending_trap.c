/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   next_pending_trap.c                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/08 00:52:29 by dlesieur          #+#    #+#             */
/*   Updated: 2025/11/08 08:48:33 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "trap.h"

int	next_pending_trap(int start)
{
	register int	i;

	i = start - 1;
	while (++i < NSIG)
		if (g_sig.pending_traps[i])
			return (i);
	return (-1);
}
