/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   utils5.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 21:11:01 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/27 16:20:47 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "parser_private.h"

/* Print a bash-compatible arithmetic error message. Three cases: if the last
   token before the `)` was followed by a nested `(`, the error is a missing
   `)` in that sub-expression; if there was at least one word we quote it as
   the "error token"; otherwise it is a bare "syntax error in arithmetic
   expression" with no location hint. */
void	handle_arith_error_print(t_shell *state,
							bool has_inner_paren,
							t_token last_word)
{
	ft_eprintf("%s: ((: ", state->ctx);
	if (has_inner_paren && last_word.start)
		ft_eprintf("missing `)' (error token is \"%.*s)\")\n",
			last_word.len, last_word.start);
	else if (last_word.start)
		ft_eprintf("syntax error in expression (error token is \"%.*s\")\n",
			last_word.len, last_word.start);
	else
		ft_eprintf("syntax error in arithmetic expression\n");
}

/* Try to consume one simple-list operator. Returns 1 (stop) if the next
   token is not a simple-list operator, 2 on error, 0 on success -- the
   operator is pushed as a child of ret and its type is stored in *out_next
   so check_newlines_and_end can decide whether end-of-input is a hard
   error (after `;` or `&`) or a continuation request. TT_NEWLINE is
   accepted here as a sequencing operator (`;` semantics): batched input
   delivery hands the lexer several lines at once, so the top-level list
   must keep parsing across newlines instead of stopping at the first one.
   The executor already runs TT_NEWLINE children as separators. */
int	push_simple_list_op_token(t_parser *parser,
									t_deque_tok *tokens,
									t_ast_node *ret,
									t_tt *out_next)
{
	t_token	tmp;
	t_tt	next;

	tmp = *(t_token *)deque_peek(&tokens->deqtok);
	next = tmp.tt;
	if (!is_simple_list_op(next) && next != TT_NEWLINE)
		return (1);
	vec_push_int(&parser->parse_stack, next);
	tmp = *(t_token *)deque_pop_start(&tokens->deqtok);
	push_token_child(ret, tmp);
	if (parser->res != RES_OK)
		return (2);
	*out_next = next;
	return (0);
}

/* After consuming a simple-list operator, skip optional newlines and check
   for end-of-input. `cmd;` at EOF is valid (no continuation needed), but
   `cmd &&` at EOF means the user wants to type the right-hand side, so we
   signal RES_GETMOREINPUT. Returns 2 if the caller should stop, 0 to go on. */
int	check_newlines_and_end(t_parser *parser,
									t_deque_tok *tokens,
									t_tt next)
{
	while (is_newline_token(tokens))
		(void)deque_pop_start(&tokens->deqtok);
	if ((next == TT_SEMICOLON || next == TT_AMPERSAND
			|| next == TT_NEWLINE) && is_end_token(tokens))
		return (2);
	if (is_end_token(tokens))
	{
		parser->res = RES_GETMOREINPUT;
		return (2);
	}
	return (0);
}

/* Wrap op_tok in an AST_TOKEN node and push it onto ret->children. Used by
   parse_proc_sub to record the `<(` or `>(` operator token as the first
   child of the proc-sub node before the command body is pushed. */
void	add_op_token_child(t_ast_node *ret, t_token op_tok)
{
	t_ast_node	op_node;

	op_node = create_node_tok(AST_TOKEN, op_tok);
	vec_init(&op_node.children);
	op_node.children.elem_size = sizeof(t_ast_node);
	vec_push(&ret->children, &op_node);
}

/* If the current token is TT_END while still inside a `<(...)` / `>(...)`,
   the substitution is unterminated. We signal RES_GETMOREINPUT, set the
   looking_for hint to `)` for the REPL prompt, and free the partially
   accumulated command string to avoid a leak. Returns 1 to let the caller
   break out of its scan loop, 0 if this was not an EOF token. */
int	proc_sub_handle_eof(t_parser *parser,
							t_deque_tok *tokens,
							t_string *cmd_str,
							t_token curr)
{
	if (curr.tt == TT_END)
	{
		parser->res = RES_GETMOREINPUT;
		tokens->looking_for = ')';
		xfree(cmd_str->ctx);
		return (1);
	}
	return (0);
}
