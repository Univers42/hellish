/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   utils.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 19:09:53 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/27 16:07:19 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "parser_private.h"

bool	is_redirect(t_tt tt);

/* True when tt is one of the two process-substitution operator tokens. Used
   in parse_simple_cmd and parse_redirect to decide whether a `<(` / `>(`
   introduces a proc-sub rather than a plain redirect. */
bool	is_proc_sub(t_tt tt)
{
	return (tt == TT_PROC_SUB_IN || tt == TT_PROC_SUB_OUT);
}

/* True when tt can legally follow a redirect operator as its target -- words
   in all flavours plus both proc-sub forms. Used by the redirect parser to
   validate the token after the `>` / `>>` / etc. */
bool	is_redirect_target(t_tt tt)
{
	return (tt == TT_WORD || tt == TT_SQWORD || tt == TT_DQWORD
		|| tt == TT_ENVVAR || tt == TT_DQENVVAR || is_proc_sub(tt));
}

/* True when tt can begin or continue a simple command: a word, any redirect
   operator (including fd-prefixed ones), or a process substitution. This is
   the set of tokens parse_simple_command greedily consumes. */
bool	is_simple_cmd_token(t_tt tt)
{
	return (tt == TT_WORD || is_redirect(tt) || is_proc_sub(tt));
}

/* True when tt is an operator that separates or sequences pipelines inside a
   compound-list. Note the typo in the name (compund) -- it matches the
   function declaration used throughout the codebase, so we leave it as-is to
   avoid a multi-file rename. */
bool	is_compund_list_op(t_tt tt)
{
	if (tt == TT_SEMICOLON
		|| tt == TT_OR
		|| tt == TT_AND
		|| tt == TT_NEWLINE
		|| tt == TT_AMPERSAND)
		return (true);
	return (false);
}

/* Wrap the next token (assumed to be a word in any flavour) in an AST_WORD >
   AST_TOKEN subtree. The token is popped from the deque. This two-level wrap
   mirrors the grammar rule: a word is a sequence of one or more tokens (the
   re-parse pass may push multiple TOKEN children for composite words). */
t_ast_node	parse_word(t_deque_tok *tokens)
{
	t_ast_node	ret;
	t_token		tmp;
	t_ast_node	token_node;

	ret = (t_ast_node){.node_type = AST_WORD};
	vec_init(&ret.children);
	ret.children.elem_size = sizeof(t_ast_node);
	tmp = *(t_token *)deque_pop_start(&tokens->deqtok);
	token_node = create_node_tok(AST_TOKEN, tmp);
	vec_init(&token_node.children);
	token_node.children.elem_size = sizeof(t_ast_node);
	ast_push_child(&ret, &token_node);
	return (ret);
}
