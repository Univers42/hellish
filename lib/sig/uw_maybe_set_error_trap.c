/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   uw_maybe_set_error_trap.c                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/08 00:54:28 by dlesieur          #+#    #+#             */
/*   Updated: 2025/11/08 06:31:47 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "trap.h"

void	uw_maybe_set_error_trap(void *command)
{
	trap_if_untrapped(ERROR_TRAP, command);
}
