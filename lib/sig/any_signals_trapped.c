/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   any_signals_trapped.c                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/08 00:51:46 by dlesieur          #+#    #+#             */
/*   Updated: 2025/11/10 14:52:53 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "trap.h"

int	any_signals_trapped(void)
{
	register int	i;

	i = 0;
	while (++i < BASH_NSIG)
		if ((get_g_sig()->sigmodes[i] & SIG_TRAPPED)
			&& (get_g_sig()->sigmodes[i] & SIG_IGNORED) == 0)
			return (i);
	return (-1);
}
