/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   trap_if_untrapped.c                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/08 00:54:17 by dlesieur          #+#    #+#             */
/*   Updated: 2025/11/08 02:58:35 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "trap.h"

/* Set a trap for SIG only if SIG is not already trapped. */
void	trap_if_untrapped(int sig, const char *command)
{
	if ((g_sig.sigmodes[sig] & SIG_TRAPPED) == 0)
		set_signal (sig, command);
}
