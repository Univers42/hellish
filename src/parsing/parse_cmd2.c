/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   parse_cmd2.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 19:11:16 by dlesieur          #+#    #+#             */
/*   Updated: 2026/03/14 18:05:04 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "parser_private.h"

/* Parse a function definition and push it as a child of ret. Only reached
   after is_function_def has confirmed the WORD ( ) pattern, so the first
   three tokens are always well-formed here. */
static bool	handle_func_def(t_shell *state, t_parser *parser,
							t_deque_tok *tokens, t_ast_node *ret)
{
	t_ast_node	func_node;

	func_node = parse_function_def(state, parser, tokens);
	vec_push(&ret->children, &func_node);
	if (parser->res != RES_OK)
		return (false);
	return (true);
}

/* Decide what kind of command starts at the current token position and call
   the appropriate handler. Priority: subshell `(` > compound keyword > func
   definition > simple command. Function-def check must come before simple
   command because `name()` starts with TT_WORD, which is also a valid simple
   command opener; is_function_def looks ahead two tokens to resolve the
   ambiguity without consuming them. */
static bool	dispatch_cmd(t_shell *state, t_parser *parser,
						t_deque_tok *tokens, t_ast_node *ret)
{
	t_tt	next;

	next = (*(t_token *)deque_peek(&tokens->deqtok)).tt;
	if (next == TT_BRACE_LEFT)
		return (handle_subshell_case(state, parser, tokens, ret));
	if (is_compound_start(next))
		return (handle_compound_case(state, parser, tokens, ret));
	if (is_function_def(tokens))
		return (handle_func_def(state, parser, tokens, ret));
	return (handle_simple_command_case(state, parser, tokens, ret));
}

/* Parse one command (simple, compound, subshell, or function definition).
   TT_ARITH_START at this level is always a syntax error -- `((` can only
   appear in arithmetic evaluation, not as a standalone command without the
   full `((...))` enclosure that the lexer would have classified correctly.
   On success dispatch_cmd fills ret.children; on failure parser->res is set
   accordingly and ret is returned incomplete for the caller to propagate. */
t_ast_node	parse_command(t_shell *state, t_parser *parser,
				t_deque_tok *tokens)
{
	t_ast_node	ret;
	t_tt		next;

	ret = (t_ast_node){.node_type = AST_COMMAND};
	vec_init(&ret.children);
	ret.children.elem_size = sizeof(t_ast_node);
	next = (*(t_token *)deque_peek(&tokens->deqtok)).tt;
	if (next == TT_ARITH_START)
	{
		parser->res = RES_ERR;
		state->last_cmd_st_exe = res_status(1);
		return (ret);
	}
	if (!dispatch_cmd(state, parser, tokens, &ret))
		return (ret);
	return (ret);
}
