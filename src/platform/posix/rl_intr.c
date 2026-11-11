/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   rl_intr.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/04 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/11/04 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "rl_private.h"
#include <signal.h>

/* What a ^C does to a read, in one place.
**
** readline 8.3 made rl_free_line_state free the context of ^R, Alt-P and
** M-x, on the bet that an application's SIGINT handler longjmps and that
** reader never runs again. bash does; readline's own rl_getc, since
** readline83-003, aborts to the top level for the handlers that return.
** This read does neither, by design (rl_getc.c): the reader is handed EOF
** and unwinds itself, through a `cxt` local that now points at freed
** memory. Issue #194, from CI on readline 8.3-4:
**
**     rl_getc_hook -> rl_check_signals -> _rl_handle_signal
**       -> rl_free_line_state -> _rl_isearch_cleanup -> free(cxt)
**     rl_search_history -> _rl_isearch_cleanup(cxt) -> free(cxt)
**
** A double free under ASan, a use after free under an instrumented
** readline for all three readers, and on a release build a SIGSEGV that
** takes the login shell, 3 runs of 3 on ^R.
**
** readline runs that cleanup wherever it next looks at its deferred
** signal, and the look at the end of every rl_redisplay is inside the
** search, so guarding this file's own look is not enough: a ^C sent while
** ^R is drawing its keys came through there, and the test that caught it
** sends one. So readline must not hear of a ^C while such a reader is up.
** A filter goes in front of its handler and parks the signal; the reader
** gets EOF and unwinds with its context intact, the path every readline
** before 8.3 took, and back in the main loop the ^C is replayed into
** readline, which handles it as one that arrived there. Outside these
** readers the filter only forwards, so ^C anywhere else is what it always
** was, a slow completion included.
**
** Caveat: the filter reads the state bit, and the bit goes up only once
** the reader's command runs. A ^C landing after readline's last look at
** the signal and before that (a few hundred instructions, no system call
** in them) reaches readline as usual and frees the context at the next
** look. Blocking SIGINT from the key's return until the next wait would
** close it, but the block would need lifting around every widget and
** completion, and a child forked on a path that misses it inherits it. A
** context a later readline adds to _rl_state_sigcleanup is not in
** RL_CXT_READERS until someone puts it there. */

static struct sigaction	*rl_readline_sigint(void)
{
	static struct sigaction	act;

	return (&act);
}

static void	rl_sigint_filter(int sig)
{
	if (rl_readline_state & RL_CXT_READERS)
		*rl_intr_cell() = RL_INTR_PARKED;
	else
		rl_readline_sigint()->sa_handler(sig);
}

/* rl_set_signals installs readline's handler at the top of readline() and
   again after a signal it handled or a widget's bracket (rl_editor.c), so
   this looks every time. Its restore puts back what it saved then, never
   the filter. A SIG_DFL here is the forked reader's child in its cleanup:
   nothing to stand in front of. */
static void	rl_sigint_arm(void)
{
	struct sigaction	act;

	if (sigaction(SIGINT, NULL, &act) != 0
		|| act.sa_handler == rl_sigint_filter
		|| act.sa_handler == SIG_DFL || act.sa_handler == SIG_IGN)
		return ;
	*rl_readline_sigint() = act;
	act.sa_handler = rl_sigint_filter;
	sigaction(SIGINT, &act, NULL);
}

/* A signal since the last look: let readline handle it now -- cleanup,
   the application's handler, re-prep -- and remember an interrupt, which
   ends this read. A parked ^C waits for its reader to be gone, and while
   it waits rl_should_abort is what makes the reader go. */
void	rl_take_signals(void)
{
	int	sig;

	if (*rl_intr_cell() == RL_INTR_PARKED
		&& !(rl_readline_state & RL_CXT_READERS))
		rl_readline_sigint()->sa_handler(SIGINT);
	sig = rl_pending_signal();
	if (sig == SIGINT)
	{
		*rl_intr_cell() = 1;
		rl_suggest_unpaint();
	}
	if (sig)
		rl_check_signals();
	rl_sigint_arm();
}
