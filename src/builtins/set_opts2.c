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

/* set -o (no option name): list every shell option with its current state,
   in the roster's own order -- which is bash's alphabetical listing order,
   so the two outputs are byte-identical. */
int	list_set_options(t_shell *state)
{
	const t_setopt	*e;

	e = setopt_table();
	while (e->name)
	{
		print_set_opt(e->name, setopt_get(state, e));
		e++;
	}
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

/* Consume one flag word and whatever it drags in with it.  Returns the number
   of argv words eaten (1, or 2 when an `o` took the following word as a long
   option name), or -1 after reporting a usage error.

   The `o` handling is the subtle part and is bash's, not an invention: `o`
   may sit anywhere in a cluster and always takes its NAME from the next word
   -- so `set -euo pipefail` sets errexit, nounset and pipefail and consumes
   two words, while `set -eo errexit x y` leaves x and y as positionals.  With
   no word left to take, `-o` lists the options instead of erroring. */
static int	set_flag_word(t_shell *state, char **w, size_t remaining)
{
	bool	want_o;

	want_o = false;
	if (!apply_flag_letters(state, w[0], &want_o))
		return (ft_eprintf("%s: set: invalid option\n", state->ctx), -1);
	if (!want_o)
		return (1);
	if (remaining < 2)
		return (list_set_options(state), 1);
	if (set_long_option(state, w[0][0], w[1]))
		return (ft_eprintf("%s: set: %s: invalid option name\n",
				state->ctx, w[1]), -1);
	return (2);
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
		if (av[i][0] != '+' && (av[i][0] != '-' || !av[i][1]))
			return (set_positional_args(state, av + i, argv.len - i));
		rc = set_flag_word(state, av + i, argv.len - i);
		if (rc < 0)
			return (2);
		i += (size_t)rc;
	}
	return (0);
}
