/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   expander_split.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 23:31:26 by marvin            #+#    #+#             */
/*   Updated: 2026/01/09 23:31:26 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"
#include "parena.h"

/* Initialise an empty AST_WORD node ready to receive child tokens.
   Used when building fresh field nodes during IFS splitting. */
void	init_word_node(t_ast_node *n)
{
	*n = (t_ast_node){.node_type = AST_WORD};
	vec_init(&n->children);
	n->children.elem_size = sizeof(t_ast_node);
}

/* Append `child` into curr_node's children vector (transfer of ownership).
   Called when a non-splitting token (quoted, or non-IFS text) stays in the
   current accumulation field rather than starting a new one. */
void	push_token_node(t_ast_node *curr_node, t_ast_node *child)
{
	vec_push(&curr_node->children, child);
}

void	ft_reset(void *ptr, size_t size, void (*cust_act_bef_reset)(void *))
{
	if (cust_act_bef_reset)
		cust_act_bef_reset(ptr);
	ft_memset(ptr, 0, size);
}

/* free children.ctx pointer of an ast node prior to zeroing the node.
   Routed through parena_free: parse-arena buffers (cycle trees) are
   reclaimed wholesale at cycle end, heap buffers are really freed. */
void	free_children(void *p)
{
	t_ast_node	*n;

	if (!p)
		return ;
	n = (t_ast_node *)p;
	if (n->children.ctx)
		parena_free(n->children.ctx);
}

/* Release a consumed subtoken's own allocations.
   split_envvar/emit_positional_at build fresh field nodes, so start
   (clone-owned) and full_word would leak if not freed here. Routed via
   parena_free because full_word copies live in the parse arena for cycle
   trees and on the heap for clones. */
void	free_token_res(t_token *t)
{
	if (t->allocated)
		parena_free((char *)t->start);
	t->allocated = false;
	if (t->full_word)
	{
		if (t->full_word->allocated)
			parena_free(t->full_word->start);
		parena_free(t->full_word);
		t->full_word = NULL;
	}
}
