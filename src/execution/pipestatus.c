/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   pipestatus.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/21 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/07/21 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "execution_private.h"
#include "env.h"

/* PIPESTATUS: an array of every stage's exit status, refreshed after each
   foreground pipeline — bash semantics, cheap now that indexed arrays
   exist (the value is one encoded string; no env-lifecycle involvement).
   Never exported; overwritten wholesale on every update. */

/* Record one status per element of the waited results vector. */
void	set_pipestatus(t_shell *state, t_vec_exe_res *results)
{
	char	**elems;
	size_t	i;
	char	*val;

	elems = xmalloc(sizeof(char *) * (results->len + 1));
	if (!elems)
		return ;
	i = 0;
	while (i < results->len)
	{
		elems[i] = ft_itoa(
				((t_execution_state *)vec_idx(results, i))->status);
		i++;
	}
	val = arr_from_elems(elems, (int)results->len, NULL);
	i = 0;
	while (i < results->len)
		xfree(elems[i++]);
	xfree(elems);
	env_set(&state->env, env_create(ft_strdup("PIPESTATUS"), val, false));
}

/* Single-command form: PIPESTATUS=(status). */
void	set_pipestatus_one(t_shell *state, int status)
{
	char	*n;
	char	*val;

	n = ft_itoa(status);
	if (!n)
		return ;
	val = arr_from_elems(&n, 1, NULL);
	xfree(n);
	env_set(&state->env, env_create(ft_strdup("PIPESTATUS"), val, false));
}
