/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   uw_maaybe_set_return_trap.c                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/08 00:54:23 by dlesieur          #+#    #+#             */
/*   Updated: 2025/11/08 06:31:09 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "trap.h"

void	uw_maybe_set_return_trap(void *command)
{
	trap_if_untrapped (RETURN_TRAP, command);
}
