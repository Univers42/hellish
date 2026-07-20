/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   parser_simple_list.c                               :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 23:32:24 by marvin            #+#    #+#             */
/*   Updated: 2026/01/09 23:32:24 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "parser_private.h"

/* Parse one pipeline and push it as a child of ret. Pops the parse_stack
   entry previously pushed by parse_simple_list_op (the op token that
   preceded this pipeline) before returning, keeping the stack balanced. */
static int	parse_and_push_pipeline(t_shell *state,
									t_parser *parser,
									t_deque_tok *tokens,
									t_ast_node *ret)
{
	t_ast_node	tmp_node;

	tmp_node = parse_pipeline(state, parser, tokens);
	vec_push(&ret->children, &tmp_node);
	if (parser->res != RES_OK)
		return (2);
	vec_pop(&parser->parse_stack);
	return (0);
}

/* Handle one iteration of the simple-list loop: consume the operator token,
   skip newlines, detect end-of-input, and parse the next pipeline. Returns
   1 when the loop should stop (no more operators), 2 on error/incomplete. */
static int	parse_simple_list_op(t_shell *state, t_parser *parser,
								t_deque_tok *tokens, t_ast_node *ret)
{
	t_tt	next;
	int		r;

	r = push_simple_list_op_token(parser, tokens, ret, &next);
	if (r != 0)
		return (r);
	r = check_newlines_and_end(parser, tokens, next);
	if (r != 0)
		return (r);
	return (parse_and_push_pipeline(state, parser, tokens, ret));
}

/* At the very end of a simple-list we expect either a newline (which we
   consume silently as the statement terminator) or TT_END (end of input,
   also clean). Anything else is a syntax error -- e.g. an unexpected `fi`
   or `done` that has no matching opener at this level. */
static void	handle_final_newline_or_end(t_shell *state, t_parser *parser,
										t_ast_node *ret,
										t_deque_tok *tokens)
{
	if (is_newline_token(tokens))
		(void)deque_pop_start(&tokens->deqtok);
	else if (!is_end_token(tokens))
		handle_unexpected_token(state, parser, *ret, tokens);
}

/* Drain simple-list operators (`; & && ||`) one at a time until the token
   stream is exhausted or an error is signalled. Status codes: 1 = done
   cleanly, 2 = error or need more input (caller checks parser->res). */
static int	process_all_simple_list_ops(t_shell *state, t_parser *parser,
										t_deque_tok *tokens, t_ast_node *ret)
{
	int	status;

	while (1)
	{
		status = parse_simple_list_op(state, parser, tokens, ret);
		if (status == 1)
			return (1);
		if (status == 2)
			return (2);
	}
}

/* Parse a simple-list: pipeline [; & && || pipeline]... [\n|EOF].
   This is the outermost grammar production -- the direct child of the
   top-level parse_tokens call. It rejects TT_ARITH_START at the start
   (a bare `((` without a matching `))` already tokenised is a hard error)
   and any token that cannot begin a command. The first pipeline is mandatory;
   subsequent op-pipeline pairs are optional. */
t_ast_node	parse_simple_list(t_shell *state, t_parser *parser,
								t_deque_tok *tokens)
{
	t_ast_node	ret;
	t_tt		next;
	int			status;

	init_ast_node_children(&ret, AST_SIMPLE_LIST);
	skip_newlines(tokens);
	next = (*(t_token *)deque_peek(&tokens->deqtok)).tt;
	if (next == TT_END)
		return (ret);
	if (!is_simple_cmd_token(next) && next != TT_BRACE_LEFT
		&& !is_compound_start(next))
		return (handle_unexpected_token(state, parser, ret, tokens), ret);
	push_parsed_pipeline_child(state, parser, tokens, &ret);
	if (parser->res != RES_OK)
		return (ret);
	status = process_all_simple_list_ops(state, parser, tokens, &ret);
	if (status == 2)
		return (ret);
	handle_final_newline_or_end(state, parser, &ret, tokens);
	return (ret);
}
