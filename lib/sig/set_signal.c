/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   set_signal.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/08 00:53:50 by dlesieur          #+#    #+#             */
/*   Updated: 2025/11/10 18:17:13 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "trap.h"

static int orig_need_fetch(int sig)
{
	if (get_g_sig()->original_signals[sig] == (void *)IMPOSSIBLE_TRAP_HANDLER)
		return (1);
	return (0);
}

static int orig_is_untrap_ignored(int sig)
{
	if (get_g_sig()->original_signals[sig] == SIG_IGN && (get_g_sig()->sigmodes[sig] & SIG_ASYNCSIG) == 0)
		return (1);
	return (0);
}

static void install_trap(int sig, const char *string,
						 t_sigset *set, t_sigset *oset)
{
	block_signal(sig, set, oset);
	change_signal(sig, savestring(string));
	set_signal_handler(sig, trap_handler);
	unblock_signal(oset);
}

void set_signal(int sig, const char *string)
{
	t_sigset set;
	t_sigset oset;

	if (spec_trap(sig))
	{
		change_signal(sig, savestring(string));
		if (sig == EXIT_TRAP && get_g_sig()->interactive == 0)
			initialize_terminating_signals();
		return;
	}
	if (get_g_sig()->sigmodes[sig] & SIG_HARD_IGNORE)
		return;
	if ((get_g_sig()->sigmodes[sig] & SIG_TRAPPED) == 0)
	{
		if (orig_need_fetch(sig))
			get_orig_sig(sig);
		if (orig_is_untrap_ignored(sig))
			return;
	}
	if ((get_g_sig()->sigmodes[sig] & SIG_NO_TRAP) == 0)
		install_trap(sig, string, &set, &oset);
	else
		change_signal(sig, savestring(string));
}
