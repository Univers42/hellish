/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   any_signals_trapped.c                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/08 00:51:46 by dlesieur          #+#    #+#             */
/*   Updated: 2025/11/08 08:48:30 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "trap.h"

// return >  0 if any of the real signals (not fake siganl like EXIT)
// are trapped
int	any_signals_trapped(void)
{
	register int	i;

	i = 0;
	while (++i < BASH_NSIG)
		if ((g_sig.sigmodes[i] & SIG_TRAPPED)
			&& (g_sig.sigmodes[i] & SIG_IGNORED) == 0)
			return (i);
	return (-1);
}
