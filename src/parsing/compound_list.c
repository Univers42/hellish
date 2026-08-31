/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   parser_compund_list.c                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 23:32:20 by marvin            #+#    #+#             */
/*   Updated: 2026/01/09 23:32:20 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "parser_private.h"
#include "ft_glob.h"

/* Consume one `op pipeline` pair inside a compound-list (e.g. `; cmd` or
   `&& cmd`). Returns true when the loop should stop: either the next token
   is not a compound-list operator, or we hit a compound terminator (do/then/
   fi/done/esac/;;) after a separator -- that's the `;` before `done` pattern.
   Pushes the operator token and the next pipeline as children of ret.
   Gotcha: we must check is_separator_before_terminator before consuming more
   input, otherwise `do ; done` would try to parse `done` as a command. */
bool	parse_compound_list_s(t_shell *state, t_parser *parser,
							t_deque_tok *tokens, t_ast_node *ret)
{
	t_tt	op;
	t_token	tmp;

	tmp = ltok2tok(*(t_ltoken *)deque_peek(&tokens->deqtok), tokens->base);
	op = tmp.tt;
	if (!is_compund_list_op(op))
		return (true);
	tmp = pop_tok(tokens);
	push_token_child(ret, tmp);
	if (is_separator_before_terminator(ret, tokens))
		return (true);
	vec_push_int(&parser->parse_stack, op);
	while ((*(t_ltoken *)deque_peek(&tokens->deqtok)).tt == TT_NEWLINE)
		(void)deque_pop_start(&tokens->deqtok);
	if (is_compound_terminator(
			(*(t_ltoken *)deque_peek(&tokens->deqtok)).tt))
		return (true);
	if ((*(t_ltoken *)deque_peek(&tokens->deqtok)).tt == TT_END)
		return (parser->res = RES_GETMOREINPUT, true);
	{
		push_parsed_pipeline_child(state, parser, tokens, ret);
		if (parser->res != RES_OK)
			return (true);
	}
	return (vec_pop(&parser->parse_stack), false);
}

/* Parse a compound-list: a sequence of pipelines separated by `; & && ||
   \n`. Used as the body of if/while/for/case/function and for subshells.
   Leading newlines are consumed first (POSIX allows `if\n\ncmd\nthen`).
   TT_ARITH_START opens an arithmetic command `(( expr ))` like any other
   pipeline start — `if ((x))` and `while ((n<3))` depend on it.
   The loop runs until parse_compound_list_s signals "done".
     An EMPTY body -- the terminator arriving where a pipeline was expected
   -- is accepted in the zsh dialect only. `if true; then` followed by
   nothing but a comment, and `f() { # todo\n}`, are both zsh; bash calls
   them a syntax error and the golden suite pins that, so the dialect gate
   is what keeps the two answers apart rather than a permissiveness that
   leaks. Comments never reach here (the lexer drops them), so what this
   really accepts is "the body is only whitespace and comments" -- which is
   how the construct is always written. */
t_ast_node	parse_compound_list(t_shell *state,
								t_parser *parser, t_deque_tok *tokens)
{
	t_ast_node	ret;
	t_tt		next;

	ret = (t_ast_node){.node_type = AST_COMPOUND_LIST};
	vec_init(&ret.children);
	ret.children.elem_size = sizeof(t_ast_node);
	skip_newlines(tokens);
	next = (*(t_ltoken *)deque_peek(&tokens->deqtok)).tt;
	if (next == TT_END)
		return (parser->res = RES_GETMOREINPUT, ret);
	if (glob_zsh() && is_compound_terminator(next))
		return (ret);
	push_parsed_pipeline_child(state, parser, tokens, &ret);
	if (parser->res != RES_OK)
		return (ret);
	while (1)
	{
		if (parse_compound_list_s(state, parser, tokens, &ret))
			break ;
	}
	return (ret);
}
