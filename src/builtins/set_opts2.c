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
