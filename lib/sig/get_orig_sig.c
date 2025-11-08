/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   get_orig_sig.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/08 02:14:50 by dlesieur          #+#    #+#             */
/*   Updated: 2025/11/08 09:01:20 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "trap.h"

void	get_orig_sig(int sig)
{
	if (sig <= 0 || sig >= NSIG)
		return ;
	g_sig.original_signals[sig] = (void *)set_signal_handler(sig, SIG_DFL);
	set_signal_handler(sig, (void (*)(int))g_sig.original_signals[sig]);
	if (g_sig.original_signals[sig] == (void *)SIG_IGN)
		g_sig.sigmodes[sig] |= SIG_HARD_IGNORE;
}
