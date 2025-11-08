/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   reset_signal.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/08 00:52:43 by dlesieur          #+#    #+#             */
/*   Updated: 2025/11/08 09:01:22 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "trap.h"

/* Reset the handler for SIG to the original value but leave the trap string
	 in place. */
void	reset_signal(int sig)
{
	if ((g_sig.sigmodes[sig] & SIG_ASYNCSIG) && signal_is_trapped (sig)
		&& g_sig.original_signals[sig] == SIG_IGN)
		g_sig.original_signals[sig] = SIG_DFL;
	set_signal_handler (sig, (void (*)(int))g_sig.original_signals[sig]);
	g_sig.sigmodes[sig] &= ~SIG_TRAPPED;
}
