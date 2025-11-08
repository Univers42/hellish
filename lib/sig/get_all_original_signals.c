/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   get_all_original_signals.c                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/08 00:52:19 by dlesieur          #+#    #+#             */
/*   Updated: 2025/11/08 05:39:54 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "trap.h"

void	get_all_original_signals(void)
{
	register int	i;

	i = 0;
	while (++i < NSIG)
		GET_ORIGINAL_SIGNAL (i);
}
