/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   set_opts.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/02 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/02 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"
#include "sh_input.h"

void	exit_clean(t_shell *state, int code);

/* Print the xtrace line (set -x behaviour). The $PS4 prefix defaults to
   "+ " (bash uses the actual string "+ "); we use that default verbatim so
   automated tests that grep for `^+ ` work without configuration. */
void	xtrace_print(t_shell *state, t_vec *argv)
{
	char	*ps4;
	size_t	i;

	ps4 = env_expand(state, "PS4");
	if (!ps4)
		ps4 = "+ ";
	ft_eprintf("%s", ps4);
	i = 0;
	while (i < argv->len)
	{
		ft_eprintf("%s", ((char **)argv->ctx)[i]);
		if (++i < argv->len)
			ft_eprintf(" ");
	}
	ft_eprintf("\n");
}

/* Called when -u (nounset) is set and an unset variable is expanded.
   Non-interactively POSIX requires the shell to exit, and the STATUS bash
   uses depends on the mode, so we mirror it exactly (harnesses diff $?):
     - errexit active    => 1 (errexit reaches the exit first)
     - -c string (INP_ARG), top-level shell => 127
     - script, piped stdin, or any forked child => 1
   Interactively we report the error but keep the REPL alive with $?=1 --
   killing the session on every mistyped variable would be unbearable. */
void	nounset_abort(t_shell *state, const char *name, int len)
{
	ft_eprintf("%s: %.*s: parameter not set\n", state->ctx, len, name);
	if (state->metinp == INP_RL)
	{
		state->last_cmd_st_exe = (t_execution_state){.status = 1};
		return ;
	}
	if (state->opt_errexit)
		exit_clean(state, 1);
	exit_clean(state, shell_fatal_status(state));
}

/* Switch the line-editor mode.  vi and emacs are mutually exclusive in bash,
   so turning one on clears the other's flag; turning one off (`set +o vi`)
   only drops the flag -- there is no "neither mode" for the editor itself,
   so edit_mode keeps whatever it had. */
void	set_opt_edit_mode(t_shell *state, const char *name, bool on)
{
	if (!on)
		return ;
	if (!ft_strcmp(name, "vi"))
	{
		state->edit_mode = 0;
		state->rl.edit_mode = 0;
		state->setopt &= ~SETOPT_EMACS;
	}
	else if (!ft_strcmp(name, "emacs"))
	{
		state->edit_mode = 1;
		state->rl.edit_mode = 1;
		state->setopt &= ~SETOPT_VI;
	}
}

/* set -o name / set +o name.  Every name bash knows is accepted, so a script
   that turns on an option hellish tracks-but-does-not-yet-honour keeps
   running instead of dying on a usage error; an unknown name returns 1 and
   the caller reports it. */
int	set_long_option(t_shell *state, char sign, const char *name)
{
	const t_setopt	*e;

	e = setopt_find(name, 0);
	if (!e)
		return (1);
	setopt_put(state, e, sign == '-');
	return (0);
}
