/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   set_trap_state.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/08 00:53:53 by dlesieur          #+#    #+#             */
/*   Updated: 2025/11/08 08:48:33 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "trap.h"

/* Set the private state variables noting that we received a signal SIG
	 for which we have a trap set. */
void	set_trap_state(int sig)
{
	g_sig.catch_flag = 1;
	g_sig.pending_traps[sig]++;
	g_sig.trapped_signal_received = sig;
}
