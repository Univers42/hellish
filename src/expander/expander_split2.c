/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   expander_split2.c                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 23:31:26 by marvin            #+#    #+#             */
/*   Updated: 2026/01/09 23:31:26 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"

/* Route one expanded subtoken to its destination field.  Deferred
   positional tokens — still holding their bare @ / * name, recognisable
   by allocated=false (an EXPANDED token whose value happens to be "@" is
   always an owned copy) — are re-emitted per positional: quoted "$@"
   verbatim, unquoted $@/$* with each element IFS-split.  TT_ENVVAR and
   split-eligible TT_WORD subtokens go through split_envvar for IFS field
   splitting.  All other tokens (literal text, single-quoted, and
   double-quoted vars) are appended to curr_node verbatim. */
/* Deferred array tokens: the marker registry proves ownership by
   pointer, the marker text is the array name. Quoted @ emits verbatim
   fields, everything else emits split fields. */
static bool	try_array_child(t_shell *state, t_token *curr_t,
				t_ast_node *curr_node, t_vec_nd *ret)
{
	char	*name;

	name = arr_mark_name(state, curr_t->start);
	if (!name)
		return (false);
	if (name[0] == '!')
		emit_keys_fields(state, name, curr_node, ret);
	else if (curr_t->tt == TT_DQENVVAR)
		emit_array_at(state, name, curr_node, ret);
	else
		emit_array_split(state, name, curr_node, ret);
	return (free_token_res(curr_t), true);
}

static void	split_one_child(t_shell *state, t_ast_node *child,
				t_ast_node *curr_node, t_vec_nd *ret)
{
	t_token	*curr_t;

	curr_t = &child->token;
	if (try_array_child(state, curr_t, curr_node, ret))
		return ;
	if (curr_t->tt == TT_DQENVVAR && curr_t->start == pos_mark('@'))
		(emit_positional_at(state, curr_node, ret), free_token_res(curr_t));
	else if (curr_t->tt == TT_ENVVAR
		&& (curr_t->start == pos_mark('@') || curr_t->start == pos_mark('*')))
		(emit_positional_split(state, curr_node, ret),
			free_token_res(curr_t));
	else if (curr_t->tt == TT_ENVVAR
		|| (curr_t->tt == TT_WORD && curr_t->split_eligible))
		(split_envvar(state, curr_t, curr_node, ret), free_token_res(curr_t));
	else if (curr_t->tt == TT_WORD || curr_t->tt == TT_SQWORD
		|| curr_t->tt == TT_DQWORD || curr_t->tt == TT_DQENVVAR)
		push_token_node(curr_node, child);
	else
		ft_assert(0);
}

/* IFS-split an already-expanded word node into a vector of field nodes.
   Each child subtoken is dispatched through split_one_child, which either
   appends it to the current accumulation field (curr_node) or finalises
   that field and starts a new one.  The original `node` is zeroed-out
   after processing so its children aren't double-freed. */
t_vec_nd	split_words(t_shell *state, t_ast_node *node)
{
	t_vec_nd	ret;
	t_ast_node	curr_node;
	int			i;

	vec_init(&ret);
	ret.elem_size = sizeof(t_ast_node);
	init_word_node(&curr_node);
	i = -1;
	while (++i < (int)node->children.len)
	{
		ft_assert(((t_ast_node *)node->children.ctx)[i].node_type
			== AST_TOKEN);
		split_one_child(state, &((t_ast_node *)node->children.ctx)[i],
			&curr_node, &ret);
	}
	if (curr_node.children.len)
		vec_push(&ret, &curr_node);
	return (ft_reset(node, sizeof(t_ast_node), free_children), ret);
}
