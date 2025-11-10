/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   set_orig_sig.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/08 02:12:24 by dlesieur          #+#    #+#             */
/*   Updated: 2025/11/10 14:50:21 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "trap.h"

void	set_orig_sig(int sig, t_sig_handler *handler)
{
	if (sig <= 0 || sig >= NSIG)
		return ;
	get_g_sig()->original_signals[sig] = handler;
	if (get_g_sig()->original_signals[sig] == SIG_IGN)
		get_g_sig()->sigmodes[sig] |= SIG_HARD_IGNORE;
}
