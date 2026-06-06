/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   reparse_word2.c                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 23:32:35 by marvin            #+#    #+#             */
/*   Updated: 2026/01/09 23:32:35 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "reparser_private.h"

/* Entry point: take a raw TT_WORD/TT_ASSIGN_WORD token from the parser
   and decompose it into a tree of typed subtokens (TT_SQWORD, TT_DQWORD,
   TT_ENVVAR, TT_WORD ...). The returned node owns its children vec; the
   caller must free_ast() it when done. This is the "second parse pass" that
   turns the flat lexer output into something the expander can act on. */
t_ast_node	reparse_word(t_token t)
{
	t_ast_node	ret;
	t_reparser	rp;

	ret = create_node_type(AST_WORD);
	vec_init(&ret.children);
	ret.children.elem_size = sizeof(t_ast_node);
	create_reparser(&rp, ret, t, &(int){0});
	loop_node_rp(&rp);
	ret = rp.current_node;
	return (ret);
}
