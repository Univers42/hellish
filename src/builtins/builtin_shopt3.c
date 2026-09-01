/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtin_shopt3.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/01 18:05:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/09/01 18:05:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"

/* Real bash option names hellish implements no behaviour for -- but that
   scripts legitimately probe. bash_completion runs `shopt -q cdable_vars`
   and `shopt -u hostcomplete` bare, so answering "invalid shell option
   name" made `. bash_completion` noisy at every login (#105). */
static bool	shopt_is_known_unimpl(const char *name)
{
	static const char	*tab[] = {"cdable_vars", "checkhash", "checkjobs",
		"cmdhist", "compat31", "direxpand", "dirspell", "execfail",
		"expand_aliases", "extdebug", "extquote", "failglob",
		"force_fignore", "globasciiranges", "globskipdots", "gnu_errfmt",
		"histreedit", "histverify", "hostcomplete", "huponexit",
		"inherit_errexit", "interactive_comments", "login_shell",
		"mailwarn", "no_empty_cmd_completion", "nocasematch",
		"patsub_replacement", "progcomp_alias", "promptvars",
		"restricted_shell", "shift_verbose", "sourcepath",
		"varredir_close", "xpg_echo", NULL};
	int					i;

	i = 0;
	while (tab[i] && ft_strcmp(tab[i], name) != 0)
		i++;
	return (tab[i] != NULL);
}

/* The honest answers cost nothing: a query or print says OFF (status 1),
   `-u` succeeds (turning off what does not run is true), and only `-s`
   stays loud -- reporting success for a behaviour that will not happen is
   the #72 anti-pattern this table must never repeat. */
static int	shopt_unimpl_act(t_shell *state, const char *name, char act,
				int quiet)
{
	if (act == 's')
		return (ft_eprintf("%s: shopt: %s: not supported\n",
				state->ctx, name), 1);
	if (act == 'u')
		return (0);
	if (!quiet && act == 'p')
		ft_printf("shopt -u %s\n", name);
	else if (!quiet)
		ft_printf("%-20s\toff\n", name);
	return (1);
}

/* Unknown-to-the-bit-table names: the known-but-unimplemented tier first,
   then bash's invalid-name error. Called by shopt_one. */
int	shopt_unknown(t_shell *state, const char *name, char act, int quiet)
{
	if (shopt_is_known_unimpl(name))
		return (shopt_unimpl_act(state, name, act, quiet));
	return (ft_eprintf("%s: shopt: %s: invalid shell option name\n",
			state->ctx, name), 1);
}
