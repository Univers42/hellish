/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   restore_original_signals.c                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/08 00:52:51 by dlesieur          #+#    #+#             */
/*   Updated: 2025/11/08 09:01:23 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "trap.h"

/* Reset all trapped signals to their original values.  Signals set to be
	 ignored with trap '' SIGNAL should be ignored, so we make sure that they
	 are.  Called by child processes after they are forked. */
void	restore_original_signals(void)
{
	reset_or_restore_signal_handlers(restore_signal);
}
