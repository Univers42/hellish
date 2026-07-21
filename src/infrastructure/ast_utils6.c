/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ast_utils6.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/20 19:00:00 by marvin            #+#    #+#             */
/*   Updated: 2026/07/20 19:00:00 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ast_private.h"
#include "parena.h"

/* Append a child node, growing the children buffer from the parse arena.
   This replaces vec_push in the parser and reparser hot paths: the buffer
   comes from parena_alloc (bump) and the outgrown one goes to parena_free,
   which no-ops arena blocks and really frees heap blocks — so a vec that
   started on the heap (or the whole call happening while the arena gate is
   closed, e.g. inside eval) behaves exactly like the old vec_push path.
   Growth first tries parena_try_extend: the array being appended to is
   usually the arena's tip (depth-first construction), so most doublings
   are a pure offset bump — no memcpy, no abandoned block.
   Clone code keeps plain vec_push: cloned trees must outlive the cycle. */
void	ast_push_child(t_ast_node *parent, t_ast_node *child)
{
	t_vec	*v;
	void	*nb;
	size_t	old;

	v = &parent->children;
	v->elem_size = sizeof(t_ast_node);
	if (v->len + 1 > v->cap)
	{
		old = v->cap * sizeof(t_ast_node);
		if (v->cap == 0)
			v->cap = 2;
		else
			v->cap = v->cap * 2;
		if (!v->ctx || !parena_try_extend(v->ctx, old,
				v->cap * sizeof(t_ast_node)))
		{
			nb = parena_alloc(v->cap * sizeof(t_ast_node));
			if (!parena()->on)
				parena_note_attach();
			if (v->len > 0)
				ft_memcpy(nb, v->ctx, v->len * sizeof(t_ast_node));
			parena_free(v->ctx);
			v->ctx = nb;
		}
	}
	ft_memcpy((char *)v->ctx + v->len * sizeof(t_ast_node),
		child, sizeof(t_ast_node));
	v->len++;
}
