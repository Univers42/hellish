/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   func_retire.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/22 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/22 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "execution_private.h"

/* Retiring a function body that might be the one currently running.

   execute_func_call walks a function straight off fn->body -- there is no
   per-call clone, and that is deliberate: it is what keeps recursion and
   loop bodies cheap. The cost is that freeing that AST while the call is
   still in flight pulls the tree out from under the walk:

       f () { if true ; then unset -f f ; fi ; }
       f

   left execute_simple_list reading children.len out of freed memory --
   a heap-use-after-free, on a script bash runs without complaint. This is
   not a contrived shape: Python's venv `deactivate` ends with
   `unset -f deactivate`, so every venv user was one deactivate away from
   a corrupted heap (found while fixing issue #39).

   So while any function call is active we unlink the body but hand it to
   dead_funcs, and free it once the outermost call has returned. Deferring
   a body that was NOT executing is harmless -- it just frees a moment
   later -- which is why the test is the cheap "is any call active" one
   rather than a search for this exact body on the call stack. */
void	retire_body(t_shell *state, t_ast_node *body)
{
	if (state->func_depth <= 0)
		return (free_ast(body));
	vec_push(&state->dead_funcs, body);
	*body = (t_ast_node){0};
}

/* Free every body retired during the call that just finished. Only safe
   once func_depth is back to 0: below that, an outer call may still be
   walking one of them. */
void	drain_dead_funcs(t_shell *state)
{
	t_ast_node	*arr;
	size_t		i;

	if (state->func_depth > 0 || state->dead_funcs.len == 0)
		return ;
	arr = (t_ast_node *)state->dead_funcs.ctx;
	i = 0;
	while (i < state->dead_funcs.len)
		free_ast(&arr[i++]);
	state->dead_funcs.len = 0;
}
