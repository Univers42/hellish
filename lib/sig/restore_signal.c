/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   restore_signal.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/08 00:52:54 by dlesieur          #+#    #+#             */
/*   Updated: 2025/11/08 03:06:20 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "trap.h"

/* Set the handler signal SIG to the original and free any trap
	 command associated with it. */
void	restore_signal(int sig)
{
	if ((g_sig.sigmodes[sig] & SIG_ASYNCSIG) && signal_is_trapped (sig)
		&& g_sig.original_signals[sig] == SIG_IGN)
		g_sig.original_signals[sig] = SIG_DFL;
	set_signal_handler (sig, g_sig.original_signals[sig]);
	change_signal (sig, (char *)DEFAULT_SIG);
	g_sig.sigmodes[sig] &= ~SIG_TRAPPED;
}
