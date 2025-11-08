/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   set_signal_hard_ignored.c                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/08 00:53:45 by dlesieur          #+#    #+#             */
/*   Updated: 2025/11/08 05:51:41 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "trap.h"

void	set_signal_hard_ignored(int sig)
{
	g_sig.sigmodes[sig] |= SIG_HARD_IGNORE;
	g_sig.original_signals[sig] = SIG_IGN;
}
