/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   set_signal_async_ignored.c                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/08 00:53:42 by dlesieur          #+#    #+#             */
/*   Updated: 2025/11/08 05:57:47 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "trap.h"

void	set_signal_async_ignored(int sig)
{
	g_sig.original_signals[sig] = SIG_IGN;
	g_sig.sigmodes[sig] |= SIG_ASYNCSIG | SIG_IGNORED;
}
