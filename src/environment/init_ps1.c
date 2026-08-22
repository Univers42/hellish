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

/* Give interactive shells a default PS1, before ~/.profile and ~/.hellishrc
   run so either can still override it.

   "\B" renders exactly what an unset PS1 used to render -- the built-in
   two-row prompt (ps1_builtin, prompt_ps1c.c) -- so the default look does
   not change at all. What changes is that PS1 now HAS a value, and that is
   what makes a Python virtualenv survive its own deactivate:

       activate:    _OLD_VIRTUAL_PS1="${PS1:-}"  ->  "\B"
                    PS1="(venv) ${PS1:-}"        ->  "(venv) \B"
       deactivate:  [ -n "$_OLD_VIRTUAL_PS1" ]   ->  true, PS1 restored

   With PS1 unset, that saved value was the empty string, deactivate's
   guard was false, and the prompt kept saying "(venv)" for the rest of the
   session no matter how many times you left the environment -- issue #39.
   Every shell that ships a default PS1 round-trips this correctly; the
   only reason hellish did not is that it had none.

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
			ft_strdup("\\B"), false));
}
