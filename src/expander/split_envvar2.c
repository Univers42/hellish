/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   split_envvar2.c                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/23 11:28:58 by dlesieur          #+#    #+#             */
/*   Updated: 2026/03/14 10:20:54 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"

/* Field splitting for a non-whitespace IFS (e.g. IFS=,): empty fields are
   preserved. Emits all fields, the first joining any accumulated text. */
static void	split_envvar_nonws(t_token *curr_t, char **things,
				t_ast_node *curr_node, t_vec_nd *ret)
{
	int	i;

	if (curr_t->allocated)
		free((char *)curr_t->start);
	curr_t->allocated = false;
	if (!things[0])
		return ((void)free(things));
	push_new_env_child(curr_node, things[0]);
	i = 1;
	while (things[i])
	{
		push_and_reinit_curr_node(ret, curr_node);
		push_new_env_child(curr_node, things[i++]);
	}
	free(things);
}

static void	distribute_fields(char **things, t_ast_node *curr_node,
				t_vec_nd *ret, bool trail)
{
	int	i;

	if (!things[0])
	{
		free(things);
		return ;
	}
	push_new_env_child(curr_node, things[0]);
	i = 1;
	while (things[i])
	{
		push_and_reinit_curr_node(ret, curr_node);
		push_new_env_child(curr_node, things[i++]);
	}
	if (trail)
		push_and_reinit_curr_node(ret, curr_node);
	free(things);
}

/*
** POSIX field splitting: leading/trailing IFS in the expanded value acts as a
** delimiter against adjacent literal text (so [$x] with x="  a  " -> [, a, ]),
** while a genuinely empty value creates no delimiter ([$x] x="" -> []).
*/
void	split_envvar(t_shell *state, t_token *curr_t,
			t_ast_node *curr_node, t_vec_nd *ret)
{
	char	**things;
	char	*ifs;
	bool	lead;
	bool	trail;

	if (!curr_t->start)
		return ;
	ifs = env_get_ifs(&state->env);
	if (ifs_has_nonws(ifs))
		return (split_envvar_nonws(curr_t,
				ifs_split_posix(curr_t->start, ifs), curr_node, ret));
	lead = is_ifs_char(curr_t->start[0], ifs);
	trail = (curr_t->start[0] != '\0'
			&& is_ifs_char(curr_t->start[ft_strlen(curr_t->start) - 1],
				ifs));
	things = ifs_split_posix(curr_t->start, ifs);
	if (curr_t->allocated)
		free((char *)curr_t->start);
	curr_t->allocated = false;
	if (lead && curr_node->children.len)
		push_and_reinit_curr_node(ret, curr_node);
	distribute_fields(things, curr_node, ret, trail);
}
