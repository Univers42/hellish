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

/* set -x : trace the command (PS4 '+ ') to stderr before it runs. */
void	xtrace_print(t_vec *argv)
{
	size_t	i;

	ft_eprintf("+");
	i = 0;
	while (i < argv->len)
	{
		ft_eprintf(" %s", ((char **)argv->ctx)[i]);
		i++;
	}
	ft_eprintf("\n");
}

/* set -u : a reference to an unset parameter is an error. */
void	nounset_abort(t_shell *state, const char *name, int len)
{
	ft_eprintf("%s: %.*s: parameter not set\n", state->ctx, len, name);
	if (state->metinp != INP_RL)
		exit_clean(state, 127);
	state->last_cmd_st_exe = (t_execution_state){.status = 1};
}

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
	else
		set_opt_edit_mode(state, name, on);
	return (0);
}

/* Apply one flag word like "-e", "+e", "-eux". */
void	apply_flag_word(t_shell *state, const char *w)
{
	char	sign;
	int		j;

	sign = w[0];
	j = 1;
	while (w[j])
	{
		if (w[j] == 'e')
			state->opt_errexit = (sign == '-');
		else if (w[j] == 'u')
			state->opt_nounset = (sign == '-');
		else if (w[j] == 'x')
			state->opt_xtrace = (sign == '-');
		else if (w[j] == 'f')
			state->opt_noglob = (sign == '-');
		else if (w[j] == 'C')
			state->opt_noclobber = (sign == '-');
		else if (w[j] == 'a')
			state->opt_allexport = (sign == '-');
		else if (w[j] == 'n')
			state->opt_noexec = (sign == '-');
		else if (w[j] == 'v')
			state->opt_verbose = (sign == '-');
		j++;
	}
}
