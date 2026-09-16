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

/* Bring readline up to date before forking. First call does the heavy
   build; later ones only notice a changed editing mode or new bindings. */
void	rl_preinit(t_rl *l)
{
	if (!l->rl_ready)
	{
		setlocale(LC_ALL, "");
		rl_instream = stdin;
		rl_outstream = stderr;
		setup_completion();
		rl_initialize();
		l->rl_ready = true;
		l->mode_applied = -1;
	}
	if (l->mode_applied != l->edit_mode)
	{
		rl_mode_apply(l->edit_mode);
		l->mode_applied = l->edit_mode;
		l->binds_done = 0;
	}
	rl_bind_pending(l);
}
