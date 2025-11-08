/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   set_original_signal.c                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/08 00:53:26 by dlesieur          #+#    #+#             */
/*   Updated: 2025/11/08 08:48:34 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "trap.h"

void	set_original_signal(int sig, t_sig_handler *handler)
{
	if (sig > 0 && sig < NSIG && g_sig.original_signals[sig]
		== (void *)IMPOSSIBLE_TRAP_HANDLER)
		SETORIGSIG (sig, handler);
}
