/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   initialize_traps.c                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/07 23:01:59 by dlesieur          #+#    #+#             */
/*   Updated: 2025/11/08 08:48:32 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "trap.h"

static inline void	init_special_traps(void)
{
	size_t			i;
	const int		traps[] = {EXIT_TRAP, DEBUG_TRAP, ERROR_TRAP, RETURN_TRAP};
	const size_t	ntraps = sizeof(traps) / sizeof(traps[0]);
	int				t;

	i = -1;
	while (++i < ntraps)
	{
		t = traps[i];
		g_sig.trap_list[t] = NULL;
		g_sig.sigmodes[t] = SIG_INHERITED;
		g_sig.original_signals[t] = (void *)IMPOSSIBLE_TRAP_HANDLER;
	}
}

static inline void	init_all_signals(void)
{
	int	i;

	i = 0;
	while (++i < NSIG)
	{
		g_sig.pending_traps[i] = 0;
		g_sig.trap_list[i] = (char *)DEFAULT_SIG;
		g_sig.sigmodes[i] = SIG_INHERITED;
		g_sig.original_signals[i] = (void *)IMPOSSIBLE_TRAP_HANDLER;
	}
}

void	init_quit_signal(void)
{
	get_orig_sig(SIGQUIT);
	g_sig.sigmodes[SIGQUIT] |= SIG_SPECIAL;
}

void	init_term_signal(void)
{
	if (g_sig.interactive)
	{
		get_orig_sig(SIGTERM);
		g_sig.sigmodes[SIGTERM] |= SIG_SPECIAL;
	}
}

void	initialize_traps(void)
{
	initialize_signames();
	init_special_traps();
	init_all_signals();
	init_chld_signal();
	init_int_signal();
	init_quit_signal();
	init_term_signal();
	get_original_tty_job_signals();
}
