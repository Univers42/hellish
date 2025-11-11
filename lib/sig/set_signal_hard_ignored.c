/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   set_signal_hard_ignored.c                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/08 00:53:45 by dlesieur          #+#    #+#             */
/*   Updated: 2025/11/10 14:45:22 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "trap.h"

void	set_signal_hard_ignored(int sig)
{
	get_g_sig()->sigmodes[sig] |= SIG_HARD_IGNORE;
	get_g_sig()->original_signals[sig] = SIG_IGN;
}
