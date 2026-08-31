/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   init_ps1.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/22 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/22 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "shell.h"
#include "env.h"
#include "libft.h"
#include "sh_input.h"
#include "prompt.h"

/* Give interactive shells a default PS1, before ~/.profile and ~/.hellishrc
   run so either can still override it.

   The value is HELLISH_PS1_DEFAULT: zsh's own default prompt --
   "hostname% ", measured on the oracle -- with the \U update badge as the
   one thing the shell still volunteers. It used to be "\B", the rich
   two-row theme, but a shell's FIRST prompt should look like the shell
   the user already knows; the theme is one `prompt` command away, not
   the thing every new user has to figure out how to turn off.

   Shipping a default value at all (rather than rendering a fallback for
   an unset PS1) is what makes a Python virtualenv survive its own
   deactivate:

       activate:    _OLD_VIRTUAL_PS1="${PS1:-}"  ->  the default
                    PS1="(venv) ${PS1:-}"        ->  "(venv) " + default
       deactivate:  [ -n "$_OLD_VIRTUAL_PS1" ]   ->  true, PS1 restored

   With PS1 unset, that saved value was the empty string, deactivate's
   guard was false, and the prompt kept saying "(venv)" for the rest of
   the session no matter how many times you left the environment --
   issue #39.

   Interactive only, so scripts, -c and piped input are untouched and the
   (entirely non-interactive) golden suite never sees a PS1 it did not set
   itself. Not exported, matching bash: PS1 is a shell variable, and
   exporting it would push our escape syntax onto every child process.
   Only filled in when the parent left a gap, like every other default. */
void	set_default_ps1(t_shell *state)
{
	if (state->metinp != INP_RL)
		return ;
	if (env_get(&state->env, "PS1"))
		return ;
	env_set(&state->env, env_create(ft_strdup("PS1"),
			ft_strdup(HELLISH_PS1_DEFAULT), false));
}
