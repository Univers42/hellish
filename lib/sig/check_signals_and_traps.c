/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   check_signals_and_traps.c                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/08 00:51:58 by dlesieur          #+#    #+#             */
/*   Updated: 2025/11/08 05:33:14 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "trap.h"

/* Convenience functions the rest of the shell can use */
void	check_signals_and_traps(void)
{
	check_signals ();
	run_pending_traps ();
}
