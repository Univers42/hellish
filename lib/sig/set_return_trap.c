/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   set_return_trap.c                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/08 00:53:28 by dlesieur          #+#    #+#             */
/*   Updated: 2025/11/08 05:58:56 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "trap.h"

void	set_return_trap(const char *command)
{
	set_signal (RETURN_TRAP, command);
}
