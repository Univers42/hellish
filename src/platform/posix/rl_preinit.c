/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   rl_preinit.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/16 16:05:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/09/16 16:05:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "rl_private.h"
#include "zle.h"
#include <locale.h>

void	setup_completion(void);
void	setup_vi_mode(void);
void	setup_emacs_mode(void);

/* Initialise readline ONCE, in the parent, instead of once per prompt.
**
** readline is read in a forked child (rl.c), and everything it needs used
** to be set up inside that child -- so every single prompt paid for a full
** rl_initialize(): the terminfo database opened and parsed, $INPUTRC or
** ~/.inputrc AND /etc/inputrc re-read, the funmap and both keymaps built
** from scratch, plus a setlocale. strace over twenty bare Enters showed
** it exactly once per Enter:
**
**     /lib/terminfo/x/xterm-256color   x20
**     ~/.inputrc                       x20
**     /etc/inputrc                     x20
**
** None of that depends on the line about to be read, and none of it is
** undone by the fork: a child inherits rl_initialized, the keymaps and the
** bindings, because they are ordinary memory. Doing it in the parent makes
** every prompt after the first inherit a readline that is already built.
**
** It is safe in the parent because rl_initialize() is the part of readline
** that touches no terminal and installs no signal handler -- rl_prep_terminal
** and rl_set_signals both run from inside readline() itself, in the child,
** which is what the fork exists to contain (rl.c's header comment).
**
** The orderings zle_install spelled out still hold, and still for the same
** reason: rl_initialize() builds the keymaps, so it comes first; the editing
** mode REPLACES the keymap, so bindings come after it -- and if the mode
** changes mid-session (`set -o vi`) the new keymap is empty of our bindings,
** which is why binds_done is rewound rather than kept. */

/* Select the editing mode's keymap. */
static void	rl_mode_apply(int edit_mode)
{
	if (edit_mode == 0)
		setup_vi_mode();
	else
		setup_emacs_mode();
}

/* Install the zle bindings this process has not installed yet. `bindkey`
   can add one at any time, so this runs per prompt and is a no-op once
   the registry has stopped growing. */
static void	rl_bind_pending(t_rl *l)
{
	t_zle_bind	*a;

	a = (t_zle_bind *)zle_binds()->ctx;
	while (l->binds_done < zle_binds()->len)
	{
		zle_bind_raw(&a[l->binds_done]);
		rl_bind_keyseq(a[l->binds_done].seq, zle_dispatch);
		l->binds_done++;
	}
}

/* HELLISH_RL_FORK=1: read in a forked child, as every release before this
   one did. Read from the process environment, once, so it works even when
   the rc file is what is broken. */
static bool	rl_fork_wanted(void)
{
	const char	*v;

	v = getenv("HELLISH_RL_FORK");
	return (v && v[0] == '1' && v[1] == '\0');
}

/* The one-time build.
**
** rl_getc_function is ours (rl_getc.c): it is how a read ends on ^C
** without a longjmp, and where the idle tick runs.
**
** rl_change_environment = 0: readline otherwise setenv()s LINES and
** COLUMNS whenever it measures the screen, and it now does that in the
** shell itself, where get_cols() reads COLUMNS first -- the width would
** stay whatever it was at the first prompt.
**
** revert-all-at-newline: history lines edited but not run are put back
** when a line is accepted. The line used to be read in a child, so such
** edits died with it; reading in-process keeps readline's history list
** across lines, and without this a recalled line changed and abandoned
** stayed changed. Bound before rl_initialize so ~/.inputrc can still
** turn it off. */
static void	rl_first_init(t_rl *l)
{
	setlocale(LC_ALL, "");
	rl_instream = stdin;
	rl_outstream = stderr;
	rl_getc_function = rl_getc_hook;
	rl_change_environment = 0;
	rl_variable_bind("revert-all-at-newline", "on");
	setup_completion();
	rl_initialize();
	l->rl_ready = true;
	l->mode_applied = -1;
	l->use_fork = !RL_INPROC_OK || rl_fork_wanted();
}

/* Bring readline up to date before a read. First call does the heavy
   build; later ones only notice a changed editing mode or new bindings,
   and re-measure the terminal, which may have been resized while a
   command ran and no readline was there to see it. */
void	rl_preinit(t_rl *l)
{
	if (!l->rl_ready)
		rl_first_init(l);
	rl_reset_screen_size();
	rl_abort_reset();
	if (l->mode_applied != l->edit_mode)
	{
		rl_mode_apply(l->edit_mode);
		l->mode_applied = l->edit_mode;
		l->binds_done = 0;
	}
	rl_bind_pending(l);
}
