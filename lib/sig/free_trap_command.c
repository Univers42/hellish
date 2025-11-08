/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   free_trap_command.c                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/08 00:52:10 by dlesieur          #+#    #+#             */
/*   Updated: 2025/11/08 05:35:16 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "trap.h"

void	free_trap_command(int sig)
{
	if ((sigmodes[sig] & SIG_TRAPPED) && trap_list[sig]
		&& (trap_list[sig] != (char *)IGNORE_SIG)
		&& (trap_list[sig] != (char *)DEFAULT_SIG)
		&& (trap_list[sig] != (char *)IMPOSSIBLE_TRAP_HANDLER))
		free (trap_list[sig]);
}
