/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   trap_to_sighandler.c                               :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/08 00:54:19 by dlesieur          #+#    #+#             */
/*   Updated: 2025/11/08 06:16:45 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "trap.h"

/* Return the correct handler for signal SIG according to the values in
	 sigmodes[SIG]. */
SigHandler	*trap_to_sighandler(int sig)
{
	if (sigmodes[sig] & (SIG_IGNORED | SIG_HARD_IGNORE))
		return (SIG_IGN);
	else if (sigmodes[sig] & SIG_TRAPPED)
		return (trap_handler);
	else
		return (SIG_DFL);
}
