/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   free_trap_string.c                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/08 00:52:12 by dlesieur          #+#    #+#             */
/*   Updated: 2025/11/08 04:02:21 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "trap.h"

/* Free a trap command string associated with SIG without changing signal
	 disposition.  Intended to be called from free_trap_strings()  */
void	free_trap_string(int sig)
{
	change_signal (sig, (char *)DEFAULT_SIG);
	g_sig.sigmodes[sig] &= ~SIG_TRAPPED;
}
