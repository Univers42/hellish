/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   initialize_traps.c                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/07 23:01:59 by dlesieur          #+#    #+#             */
/*   Updated: 2025/11/07 23:22:57 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "trap.h"

static inline void init_special_traps(void)
{
	size_t	i;
	int		traps[] = { EXIT_TRAP, DEBUG_TRAP, ERROR_TRAP, RETURN_TRAP };
	size_t	ntraps = sizeof(traps) / sizeof(traps[0]);
	int		t;

	i = -1;
	while (++i < ntraps)
	{
		t = traps[i];
		g_sig.trap_list[t] = NULL;
		g_sig.sigmodes[t] = SIG_INHERITED;
		g_sig.original_signals[t] = IMPOSSIBLE_TRAP_HANDLER;
	}
}