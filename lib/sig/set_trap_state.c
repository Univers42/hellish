/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   set_trap_state.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/08 00:53:53 by dlesieur          #+#    #+#             */
/*   Updated: 2025/11/10 14:45:14 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "trap.h"

void	set_trap_state(int sig)
{
	get_g_sig()->catch_flag = 1;
	get_g_sig()->pending_traps[sig]++;
	get_g_sig()->trapped_signal_received = sig;
}
