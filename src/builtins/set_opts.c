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
     - -c string (INP_ARG) => 127
     - script / piped stdin => 1
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
	if (state->metinp == INP_ARG)
		exit_clean(state, 127);
	exit_clean(state, 1);
}

/* Switch the line-editor mode. Only "vi" and "emacs" are recognised; +vi
   or +emacs with `on=false` is ignored (no "neither mode" concept here). */
static void	set_opt_edit_mode(t_shell *state, const char *name, bool on)
{
	if (!ft_strcmp(name, "vi") && on)
	{
		state->edit_mode = 0;
		state->rl.edit_mode = 0;
	}
	else if (!ft_strcmp(name, "emacs") && on)
	{
		state->edit_mode = 1;
		state->rl.edit_mode = 1;
	}
}

/* set -o name / set +o name. */
int	set_long_option(t_shell *state, char sign, const char *name)
{
	bool	on;

	on = (sign == '-');
	if (!ft_strcmp(name, "errexit"))
		state->opt_errexit = on;
	else if (!ft_strcmp(name, "nounset"))
		state->opt_nounset = on;
	else if (!ft_strcmp(name, "xtrace"))
		state->opt_xtrace = on;
	else if (!ft_strcmp(name, "noglob"))
		state->opt_noglob = on;
	else if (!ft_strcmp(name, "noclobber"))
		state->opt_noclobber = on;
	else if (!ft_strcmp(name, "allexport"))
		state->opt_allexport = on;
	else if (!ft_strcmp(name, "noexec"))
		state->opt_noexec = on;
	else if (!ft_strcmp(name, "verbose"))
		state->opt_verbose = on;
	else if (!ft_strcmp(name, "pipefail"))
		state->opt_pipefail = on;
	else
		set_opt_edit_mode(state, name, on);
	return (0);
}
