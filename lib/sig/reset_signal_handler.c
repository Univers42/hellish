/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   reset_signal_handler.c                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/08 00:52:40 by dlesieur          #+#    #+#             */
/*   Updated: 2025/11/08 06:07:05 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "trap.h"

/* Reset trapped signals to their original values, but don't free the
	 trap strings.  Called by the command substitution code and other places
	 that create a "subshell environment". */
void	reset_signal_handlers(void)
{
	reset_or_restore_signal_handlers (reset_signal);
}
