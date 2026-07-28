/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   set_opts2.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/02 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/02 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"
#include "sh_input.h"

/* One `set -o` line: option name left-padded then on/off, like bash --posix. */
static void	print_set_opt(const char *name, int on)
{
	if (on)
		ft_printf("%-15s\ton\n", name);
	else
		ft_printf("%-15s\toff\n", name);
}

/* set -o (no option name): list every shell option with its current state. */
int	list_set_options(t_shell *state)
{
	print_set_opt("allexport", state->opt_allexport);
	print_set_opt("errexit", state->opt_errexit);
	print_set_opt("noclobber", state->opt_noclobber);
	print_set_opt("noexec", state->opt_noexec);
	print_set_opt("noglob", state->opt_noglob);
	print_set_opt("nounset", state->opt_nounset);
	print_set_opt("verbose", state->opt_verbose);
	print_set_opt("xtrace", state->opt_xtrace);
	print_set_opt("pipefail", state->opt_pipefail);
	print_set_opt("posix", state->opt_posix);
	print_set_opt("emacs", state->edit_mode == 1);
	print_set_opt("vi", state->edit_mode == 0);
	return (0);
}

/* A leading lone `-` ends option processing and — a historic Bourne quirk
   both bash and dash keep — turns OFF xtrace and verbose.  Words after it
   become the new positional parameters, but unlike `--` a trailing lone `-`
   leaves the current positionals untouched (`set + -` keeps $@ as-is). */
static int	set_lone_dash(t_shell *state, t_vec argv, size_t from)
{
	state->opt_xtrace = false;
	state->opt_verbose = false;
	if (from < argv.len)
		return (set_positional_args(state,
				(char **)argv.ctx + from, argv.len - from));
	return (0);
}

/* Consume one option word: `-o`/`+o` (plus the name that may follow, via
   set_o_word), a ±letter cluster, or the lone `+` no-op.  Advances *i past
   what was eaten; returns 2 on an unimplemented letter so the caller can
   emit bash's "invalid option" error, 0 otherwise. */
static int	set_flag_arg(t_shell *state, char **av, size_t len, size_t *i)
{
	if (!ft_strcmp(av[*i], "-o") || !ft_strcmp(av[*i], "+o"))
		*i += set_o_word(state, av[*i][0], av + *i, len - *i);
	else if (av[*i][1])
	{
		if (!apply_flag_word(state, av[*i]))
			return (ft_eprintf("%s: set: invalid option\n",
					state->ctx), 2);
		(*i)++;
	}
	else
		(*i)++;
	return (0);
}

/* POSIX `set` argument scan: flag words are consumed left to right until
   `--`, a lone `-`, or the first non-option word.  `--` always replaces the
   positional parameters — clearing them when nothing follows; a lone `+` is
   an ignored flag, not an argument and not a terminator; `-o name`/`+o
   name` consume two words.  Everything after a terminator becomes $1..
   verbatim, even words that look like options. */
int	apply_set_flags(t_shell *state, t_vec argv)
{
	char	**av;
	size_t	i;
	int		rc;

	av = (char **)argv.ctx;
	rc = 0;
	i = 1;
	while (i < argv.len)
	{
		if (ft_strcmp(av[i], "--") == 0)
			return (set_positional_args(state, av + i + 1,
					argv.len - i - 1));
		if (ft_strcmp(av[i], "-") == 0)
			return (set_lone_dash(state, argv, i + 1));
		if (av[i][0] == '+' || (av[i][0] == '-' && av[i][1]))
			rc = set_flag_arg(state, av, argv.len, &i);
		else
			return (set_positional_args(state, av + i, argv.len - i));
		if (rc)
			return (rc);
	}
	return (0);
}
