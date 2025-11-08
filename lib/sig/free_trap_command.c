/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   free_trap_command.c                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/08 00:52:10 by dlesieur          #+#    #+#             */
/*   Updated: 2025/11/08 08:48:31 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "trap.h"

void	free_trap_command(int sig)
{
	if ((g_sig.sigmodes[sig] & SIG_TRAPPED) && g_sig.trap_list[sig]
		&& (g_sig.trap_list[sig] != (char *)IGNORE_SIG)
		&& (g_sig.trap_list[sig] != (char *)DEFAULT_SIG)
		&& (g_sig.trap_list[sig] != (char *)IMPOSSIBLE_TRAP_HANDLER))
		free (g_sig.trap_list[sig]);
}
