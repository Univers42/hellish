/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtin_history2.c                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/22 12:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/22 12:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"
#include "history.h"
#include <readline/history.h>

/* The list-mutating half of `history`: -c and -d, plus the two primitives
   every other option is built out of.

   state->hist.hist_cmds is the truth. readline keeps its own copy so the
   arrow keys can recall, and the two must move together or `history` and
   the up-arrow start disagreeing -- which is exactly what `history -c`
   used to do (it emptied our vector and left readline's list intact). */

/* A shell started with -c or a script never calls init_history, so the
   vector arrives zeroed -- elem_size 0 included, which would make the very
   first vec_push write nothing. Claim it lazily instead of forcing every
   non-interactive shell to pay for a history it will not use. */
void	hist_list_init(t_shell *state)
{
	if (state->hist.hist_cmds.elem_size != 0)
		return ;
	vec_init(&state->hist.hist_cmds);
	state->hist.hist_cmds.elem_size = sizeof(char *);
}

/* Append one already-owned entry to the list, and mirror it into readline
   when a session history is actually live. In -c / script mode readline is
   never initialised, so we would only be filling a list nothing reads. */
void	hist_push(t_shell *state, char *owned)
{
	hist_list_init(state);
	vec_push(&state->hist.hist_cmds, &owned);
	if (state->hist.hist_active)
		add_history_line(state, owned);
}

/* Drop readline's copy of entry idx so recall agrees with `history`. */
void	hist_rl_remove(t_shell *state, int idx)
{
	HIST_ENTRY	*e;

	if (!state->hist.hist_active)
		return ;
	e = remove_history(idx);
	if (e)
		free_history_entry(e);
}

/* history -c : wipe the list. The backing allocation stays so the vector is
   reusable, and `appended` goes back to zero because a later `history -a`
   must not think the entries it already wrote are still ahead of it. */
int	hist_clear(t_shell *state)
{
	size_t	i;

	i = 0;
	while (i < state->hist.hist_cmds.len)
		xfree(((char **)state->hist.hist_cmds.ctx)[i++]);
	state->hist.hist_cmds.len = 0;
	state->hist.appended = 0;
	if (state->hist.hist_active)
		clear_history();
	return (0);
}

/* history -d offset : delete one entry. Offsets are 1-based like the
   listing, and a negative one counts back from the end (-1 is the newest).
   Anything outside that is bash's "history position out of range", 1. */
int	hist_delete(t_shell *state, t_vec argv, int first)
{
	char	**av;
	int		off;
	int		len;

	av = (char **)argv.ctx;
	len = (int)state->hist.hist_cmds.len;
	if (first >= (int)argv.len)
		return (ft_eprintf("%s: history: -d: option requires an"
				" argument\n", state->ctx), 2);
	if (ft_checked_atoi(av[first], &off, 2) < 0)
		off = 0;
	else if (off < 0)
		off += len + 1;
	if (off < 1 || off > len)
		return (ft_eprintf("%s: history: %s: history position out of"
				" range\n", state->ctx, av[first]), 1);
	xfree(((char **)state->hist.hist_cmds.ctx)[off - 1]);
	ft_memmove((char **)state->hist.hist_cmds.ctx + off - 1,
		(char **)state->hist.hist_cmds.ctx + off,
		(size_t)(len - off) * sizeof(char *));
	state->hist.hist_cmds.len--;
	return (hist_rl_remove(state, off - 1), 0);
}
