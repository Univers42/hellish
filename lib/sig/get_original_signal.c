/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   get_original_signal.c                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/08 00:52:21 by dlesieur          #+#    #+#             */
/*   Updated: 2025/11/08 06:53:18 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "trap.h"

void	get_original_signal(int sig)
{
	if (sig > 0 && sig < NSIG && original_signals[sig]
		== (SigHandler *)IMPOSSIBLE_TRAP_HANDLER)
		get_orig_sig(sig);
}
