/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   sig_entry.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/14 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/09/14 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "execution_private.h"
#include "signals.h"
#include "pal.h"
#include <signal.h>

/* What the shell was handed, before it touched anything.

   POSIX, and bash: "Signals that were ignored on entry to the shell cannot
   be trapped or reset."  The shell does not get to decide this; whoever
   started it did.  A shell started as a background job is the everyday
   case -- the & that starts it sets SIGINT and SIGQUIT to SIG_IGN so a ^C
   aimed at the foreground job cannot kill it -- and so is anything run
   from cron, from a systemd unit, or from a test harness that backgrounds
   the run.

   hellish took the signal anyway, three times over: it installed its own
   SIGINT handler at startup, `trap` armed a handler for it, and a forked
   child reset it to the default before exec.  Running the golden suite in
   the background is enough to see all of it -- two `trap -p` cases
   disagreed with bash 5.3.9, and tests/hard/11_trap_signals.sh killed
   itself with its own `kill -INT $$` after a `trap - INT` that bash treats
   as a no-op.

   The dispositions are read once, as the first thing on() does, into a
   bitmap, because t_shell does not exist that early.  Signals from 64 up
   are not tracked: SH_NSIG is 65, so the only one outside is SIGRTMAX, and
   nothing hands a shell an ignored SIGRTMAX. */
static unsigned long long	g_ign_on_entry;

static void	record_entry_signals(void)
{
	struct sigaction	old;
	int					sig;

	g_ign_on_entry = 0;
	sig = 0;
	while (++sig < SH_NSIG && sig < 64)
	{
		if (sigaction(sig, NULL, &old) == 0 && old.sa_handler == SIG_IGN)
			g_ign_on_entry |= 1ULL << sig;
	}
}

/* True when this signal arrived ignored, and is therefore not ours to
   trap, to reset, or to report as anything but ''. */
int	pal_sig_ignored_on_entry(int sig)
{
	if (sig <= 0 || sig >= 64)
		return (0);
	return ((g_ign_on_entry >> sig) & 1ULL);
}

/* Install the shell's own SIGINT handler -- unless SIGINT is one of the
   signals we were told to ignore, in which case there is nothing to
   install: an ignored signal is never delivered, and a handler here would
   only be a promise the shell is not allowed to keep. */
void	pal_arm_unwind(int norestart)
{
	if (pal_sig_ignored_on_entry(SIGINT))
		return ;
	if (norestart)
		set_unwind_sig_norestart();
	else
		set_unwind_sig();
}

/* Startup: read the dispositions, then arm what we may. */
void	pal_entry_signals(void)
{
	record_entry_signals();
	pal_arm_unwind(0);
}

/* Record the ignored ones as traps of '' -- which is what they are, and
   what bash lists them as -- so `trap` and `trap -p` say so instead of
   staying silent about a signal this shell will never act on. */
void	seed_entry_traps(t_shell *state)
{
	int	sig;

	sig = 0;
	while (++sig < SH_NSIG)
	{
		if (pal_sig_ignored_on_entry(sig) && !state->traps[sig])
			state->traps[sig] = ft_strdup("");
	}
}
