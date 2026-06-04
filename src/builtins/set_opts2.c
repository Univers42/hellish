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
	print_set_opt("emacs", state->edit_mode == 1);
	print_set_opt("vi", state->edit_mode == 0);
	return (0);
}

/* set -e/-u/-x [...] : consume leading flag words. */
int	apply_set_flags(t_shell *state, t_vec argv)
{
	char	**av;
	size_t	i;

	av = (char **)argv.ctx;
	i = 1;
	while (i < argv.len && (av[i][0] == '-' || av[i][0] == '+')
		&& av[i][1] && ft_strcmp(av[i], "--") != 0)
	{
		apply_flag_word(state, av[i]);
		i++;
	}
	if (i < argv.len && ft_strcmp(av[i], "--") == 0)
		i++;
	if (i < argv.len)
		set_positional_args(state, av + i, argv.len - i);
	return (0);
}

/* Build the value of $- : one letter per currently-set option flag (POSIX). */
char	*build_flagstr(t_shell *state)
{
	int	k;

	k = 0;
	if (state->opt_allexport)
		state->flagbuf[k++] = 'a';
	if (state->opt_errexit)
		state->flagbuf[k++] = 'e';
	if (state->opt_noglob)
		state->flagbuf[k++] = 'f';
	if (state->metinp == INP_RL)
		state->flagbuf[k++] = 'i';
	if (state->opt_noexec)
		state->flagbuf[k++] = 'n';
	if (state->opt_nounset)
		state->flagbuf[k++] = 'u';
	if (state->opt_verbose)
		state->flagbuf[k++] = 'v';
	if (state->opt_xtrace)
		state->flagbuf[k++] = 'x';
	if (state->opt_noclobber)
		state->flagbuf[k++] = 'C';
	state->flagbuf[k] = '\0';
	return (state->flagbuf);
}
