/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   set_original_signal.c                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/08 00:53:26 by dlesieur          #+#    #+#             */
/*   Updated: 2025/11/08 05:59:41 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "trap.h"

void	set_original_signal(int sig, SigHandler *handler)
{
	if (sig > 0 && sig < NSIG && original_signals[sig]
		== (SigHandler *)IMPOSSIBLE_TRAP_HANDLER)
		SETORIGSIG (sig, handler);
}
