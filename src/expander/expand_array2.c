/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   expand_array2.c                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/21 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/07/21 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"

/* The array-deferral registry: ${arr[@]} in a split context replaces its
   token text with a marker string holding the ARRAY NAME, and the
   splitter recognises the marker by POINTER identity — the same trick as
   pos_mark, but with a payload, so several different arrays can defer in
   one word. Markers live in state->arr_marks and are freed wholesale at
   the start of the next simple-command expansion (and at session end):
   they are always consumed within the same expand_word pass, the late
   clear just makes aborted expansions leak-proof. */

/* Register a fresh marker carrying `name` (nlen bytes); returns it. */
char	*arr_mark_push(t_shell *state, const char *name, int nlen)
{
	char	*m;

	if (state->arr_marks.elem_size == 0)
	{
		vec_init(&state->arr_marks);
		state->arr_marks.elem_size = sizeof(char *);
	}
	m = ft_strndup(name, nlen);
	vec_push(&state->arr_marks, &m);
	return (m);
}

/* Is `p` one of our live markers? Returns the array name or NULL. */
char	*arr_mark_name(t_shell *state, const char *p)
{
	size_t	i;

	i = 0;
	while (i < state->arr_marks.len)
	{
		if (((char **)state->arr_marks.ctx)[i] == p)
			return ((char *)p);
		i++;
	}
	return (NULL);
}

/* Free every live marker and reset the registry. */
void	arr_marks_clear(t_shell *state)
{
	size_t	i;

	i = 0;
	while (i < state->arr_marks.len)
		xfree(((char **)state->arr_marks.ctx)[i++]);
	xfree(state->arr_marks.ctx);
	vec_init(&state->arr_marks);
	state->arr_marks.elem_size = sizeof(char *);
}
