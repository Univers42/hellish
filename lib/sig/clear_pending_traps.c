/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   clear_pending_traps.c                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/08 00:52:03 by dlesieur          #+#    #+#             */
/*   Updated: 2025/11/08 02:22:45 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "trap.h"

void	clear_pending_traps(void)
{
	register int	i;

	i = 0;
	while (++i < NSIG)
		g_sig.pending_traps[i] = 0;
}
