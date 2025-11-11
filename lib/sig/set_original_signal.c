/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   set_original_signal.c                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/08 00:53:26 by dlesieur          #+#    #+#             */
/*   Updated: 2025/11/10 17:59:26 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "trap.h"

void	set_original_signal(int sig, t_sig_handler *handler)
{
	if (sig > 0 && sig < NSIG
		&& get_g_sig()->original_signals[sig]
		== (void *)IMPOSSIBLE_TRAP_HANDLER)
		set_orig_sig(sig, handler);
}
