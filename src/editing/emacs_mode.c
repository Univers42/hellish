/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   emacs_mode.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/03/16 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/03/16 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

/* Readline emacs editing mode setup.  rl_editing_mode=1 is the emacs
   constant (0 is vi).  The rl_variable_bind calls set the same thing via
   the text API -- belt-and-suspenders because some readline versions
   only honour one or the other, and it costs nothing. */

#include <readline/readline.h>

/* Activate emacs key bindings.  This is the default readline mode, but
   we set it explicitly so the user's /etc/inputrc can't override us
   without the shell's explicit --vi flag being involved. */
void	setup_emacs_mode(void)
{
	rl_editing_mode = 1;
	rl_variable_bind("editing-mode", "emacs");
	rl_variable_bind("keymap", "emacs");
}
