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

/* Non-whitespace IFS splitting (e.g. IFS=, or IFS=:).  Unlike whitespace
   IFS, empty fields are preserved: "a,,b" splits into "a", "", "b".
   The first split piece joins the existing curr_node accumulation so that
   literal"$var"literal fuses correctly around the variable boundary. */
static void	split_envvar_nonws(t_token *curr_t, char **things,
				t_ast_node *curr_node, t_vec_nd *ret)
{
	int	i;

	if (curr_t->allocated)
		parena_free((char *)curr_t->start);
	curr_t->allocated = false;
	if (!things[0])
		return ((void)xfree(things));
	push_new_env_child(curr_node, things[0]);
	i = 1;
	while (things[i])
	{
		push_and_reinit_curr_node(ret, curr_node);
		push_new_env_child(curr_node, things[i++]);
	}
	xfree(things);
}

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
		parena_free((char *)curr_t->start);
	curr_t->allocated = false;
	if (lead && curr_node->children.len)
		push_and_reinit_curr_node(ret, curr_node);
	distribute_fields(things, curr_node, ret, trail);
}
