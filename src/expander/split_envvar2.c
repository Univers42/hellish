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
#include "parena.h"

static void	distribute_fields(char **things, t_ast_node *curr_node,
				t_vec_nd *ret, bool trail)
{
	int	i;

	if (!things[0])
	{
		xfree(things);
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
	xfree(things);
}

/* POSIX IFS field splitting for a single expanded variable token.
   When IFS has non-whitespace chars, use split_envvar_nonws (empty-field
   preserving).  Otherwise (default IFS " \t\n"):
     - leading IFS chars in the value split it from any preceding literal
       text (push curr_node, start fresh)
     - trailing IFS chars in the value split it from any following literal
       text (distribute_fields passes trail=true, which pushes an extra node)
   A genuinely empty value creates no split: [$x] with x="" stays as one
   empty field if surrounded by literals, or zero fields on its own. */
/* IFS-split one already-expanded VALUE and distribute the pieces as
   fields. Uniform boundary rules for every IFS flavour:
     - a leading IFS-WHITESPACE char breaks off any preceding literal
       (leading non-whitespace separators already yield an empty first
       piece from ifs_split_posix, which does the same job);
     - a trailing IFS char of EITHER class closes the final field, so a
       following literal starts fresh: IFS=:; w=a:; ${w}:b -> 'a' ':b'. */
void	split_value(t_shell *state, const char *val,
			t_ast_node *curr_node, t_vec_nd *ret)
{
	char	**things;
	char	*ifs;
	bool	trail;

	if (!val)
		return ;
	ifs = env_get_ifs(&state->env);
	trail = (val[0] != '\0'
			&& is_ifs_char(val[ft_strlen(val) - 1], ifs));
	things = ifs_split_posix(val, ifs);
	if (is_ws_ifs(val[0], ifs) && curr_node->children.len)
		push_and_reinit_curr_node(ret, curr_node);
	distribute_fields(things, curr_node, ret, trail);
}

void	split_envvar(t_shell *state, t_token *curr_t,
			t_ast_node *curr_node, t_vec_nd *ret)
{
	char	*owned;

	if (!curr_t->start)
		return ;
	owned = NULL;
	if (curr_t->allocated)
		owned = (char *)curr_t->start;
	curr_t->allocated = false;
	split_value(state, curr_t->start, curr_node, ret);
	parena_free(owned);
}
