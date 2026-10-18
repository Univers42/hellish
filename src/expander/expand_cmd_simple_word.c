/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   expand_cmd_simple_word.c                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 19:29:56 by marvin            #+#    #+#             */
/*   Updated: 2026/01/22 19:29:56 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"

/* Expand one AST_WORD child of a simple command into argv.  The word slab
   is activated (word_slab_push(1)) for the duration so that argv strings
   use the per-command slab allocator instead of plain malloc — this makes
   bulk-freeing the argv after execution much cheaper.  The `decl` flag is
   set when the command is a declaration utility (decl_word_kind), directly
   or through a bare `command`: its NAME=value arguments then expand as one
   field each (`local v=$1`, `export PATH=$PATH`). */
int	expand_simple_cmd_word(t_shell *state,
		t_expander_simple_cmd *exp, t_executable_cmd *ret)
{
	int	o;
	int	kind;

	if (!exp->found_first || exp->decl_next)
	{
		kind = decl_word_kind(*exp->curr);
		exp->decl = (kind == DECL_UTILITY);
		exp->decl_next = (!exp->found_first && kind == DECL_COMMAND);
	}
	o = word_slab_push(1);
	expand_word_ro(state, exp->curr, &ret->argv, false);
	word_slab_push(o);
	if (exec_aborting(state))
		return (1);
	exp->found_first = true;
	return (0);
}
