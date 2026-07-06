/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   parse_cmd.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 19:11:16 by dlesieur          #+#    #+#             */
/*   Updated: 2026/03/14 18:05:04 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "parser_private.h"

/* Parse a subshell (parenthesised list) and any trailing redirects. Called
   when the next token is TT_BRACE_LEFT (the `(` operator). We push the
   subshell node then greedily consume redirections -- `(cmd) >out` is
   valid syntax and the redirect applies to the entire subshell. Returns
   false if the subshell or any redirect fails to parse. */
bool	handle_subshell_case(t_shell *state, t_parser *parser,
								t_deque_tok *tokens, t_ast_node *ret)
{
	t_ast_node	tmp_node;

	tmp_node = parse_subshell(state, parser, tokens);
	vec_push(&ret->children, &tmp_node);
	if (parser->res != RES_OK)
		return (false);
	while (is_redirect((*(t_token *)deque_peek(&tokens->deqtok)).tt))
	{
		tmp_node = parse_redirect(state, parser, tokens);
		vec_push(&ret->children, &tmp_node);
		if (parser->res != RES_OK)
			return (false);
	}
	return (true);
}

/* Route to the right compound-command parser based on the leading keyword.
   TT_LBRACE goes to parse_brace_group (`{ list; }` -- runs in current shell),
   not to the subshell path (which uses TT_BRACE_LEFT, i.e. `(`). For
   everything else the mapping is 1:1 with the grammar production. */
static t_ast_node	dispatch_compound(t_shell *state, t_parser *parser,
									t_deque_tok *tokens, t_tt next)
{
	if (next == TT_IF)
		return (parse_if_command(state, parser, tokens));
	if (next == TT_WHILE)
		return (parse_while_command(state, parser, tokens));
	if (next == TT_UNTIL)
		return (parse_until_command(state, parser, tokens));
	if (next == TT_CASE)
		return (parse_case_command(state, parser, tokens));
	if (next == TT_LBRACE)
		return (parse_brace_group(state, parser, tokens));
	if (next == TT_ARITH_START)
		return (parse_arith_command(state, parser, tokens));
	return (parse_for_command(state, parser, tokens));
}

/* Parse a compound command (if/while/until/for/case/brace-group) and any
   trailing redirects, then push everything as children of ret. The trailing-
   redirect loop mirrors the subshell case: compound commands can also be
   redirected as a whole (`while ...; done >log`). */
bool	handle_compound_case(t_shell *state, t_parser *parser,
								t_deque_tok *tokens, t_ast_node *ret)
{
	t_ast_node	tmp_node;
	t_tt		next;

	next = (*(t_token *)deque_peek(&tokens->deqtok)).tt;
	tmp_node = dispatch_compound(state, parser, tokens, next);
	vec_push(&ret->children, &tmp_node);
	if (parser->res != RES_OK)
		return (false);
	while (is_redirect((*(t_token *)deque_peek(&tokens->deqtok)).tt))
	{
		tmp_node = parse_redirect(state, parser, tokens);
		vec_push(&ret->children, &tmp_node);
		if (parser->res != RES_OK)
			return (false);
	}
	return (true);
}

/* Parse a plain simple command and push it as a child of ret. This is the
   default branch -- everything that is not a subshell, a compound command,
   or a function definition falls here. */
bool	handle_simple_command_case(t_shell *state, t_parser *parser,
									t_deque_tok *tokens, t_ast_node *ret)
{
	t_ast_node	tmp_node;

	tmp_node = parse_simple_command(state, parser, tokens);
	vec_push(&ret->children, &tmp_node);
	if (parser->res != RES_OK)
		return (false);
	return (true);
}
