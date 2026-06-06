/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtin_history.c                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/02 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/02 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"
#include "history.h"

/* history -c : drop every entry but keep the vector usable (elem_size/cap). */
static void	clear_history_list(t_shell *state)
{
	size_t	i;

	i = 0;
	while (i < state->hist.hist_cmds.len)
		xfree(((char **)state->hist.hist_cmds.ctx)[i++]);
	state->hist.hist_cmds.len = 0;
}

static int	history_start(t_shell *state, t_vec argv)
{
	char	**av;
	int		n;

	if (argv.len < 2)
		return (0);
	av = (char **)argv.ctx;
	n = ft_atoi(av[1]);
	if (n > 0 && n < (int)state->hist.hist_cmds.len)
		return ((int)state->hist.hist_cmds.len - n);
	return (0);
}

/* history [-c] [n] : list the command history (optionally only the last n). */
int	builtin_history(t_shell *state, t_vec argv)
{
	char	**av;
	int		i;

	av = (char **)argv.ctx;
	if (argv.len >= 2 && !ft_strcmp(av[1], "-c"))
		return (clear_history_list(state), 0);
	i = history_start(state, argv);
	while (i < (int)state->hist.hist_cmds.len)
	{
		ft_printf("%5d  %s\n", i + 1,
			((char **)state->hist.hist_cmds.ctx)[i]);
		i++;
	}
	return (0);
}
