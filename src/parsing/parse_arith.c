/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   parse_arith.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 19:52:24 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/27 16:20:29 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "parser_private.h"

/* Consume a TT_BRACE_LEFT inside `(( ))` during error recovery and append
   a `(` to the expression buffer so the error message can reconstruct what
   the user typed. We also bump depth so we keep scanning until the matching
   `)` closes this inner pair. */
static void	append_paren_and_inc(t_deque_tok *tokens,
							t_string *expr_buf,
							int *depth)
{
	(void)expr_buf;
	(void)tokens;
	(*depth)++;
	(void)deque_pop_start(&tokens->deqtok);
	vec_push_str(expr_buf, "(");
}

/* Handle a `)` during arithmetic error recovery. If depth goes to 0 we are
   at the closing `))` -- consume it and the extra `)` that makes the double.
   If depth stays positive, this is an inner `)`, append it to the expression
   buffer so the error message shows the full expression. */
static void	handle_right_brace(t_deque_tok *tokens,
							t_string *expr_buf,
							int *depth)
{
	t_token	peek;

	(*depth)--;
	if (*depth > 0)
	{
		(void)deque_pop_start(&tokens->deqtok);
		vec_push_str(expr_buf, ")");
		return ;
	}
	(void)deque_pop_start(&tokens->deqtok);
	peek = *(t_token *)deque_peek(&tokens->deqtok);
	if (peek.tt == TT_BRACE_RIGHT)
		(void)deque_pop_start(&tokens->deqtok);
	*depth = 0;
}

/* Pull one word token from the arithmetic expression and append it to
   expr_buf (with a space separator if expr_buf is not empty). We also
   update last_word so that handle_arith_error_print can quote the token
   that was at or near the parse error site. */
static void	collect_word_token(t_deque_tok *tokens,
							t_string *expr_buf,
							t_token *last_word)
{
	t_token	peek;

	peek = *(t_token *)deque_peek(&tokens->deqtok);
	*last_word = peek;
	(void)deque_pop_start(&tokens->deqtok);
	if (expr_buf->len > 0)
		vec_push_char(expr_buf, ' ');
	vec_push_nstr(expr_buf, peek.start, peek.len);
}

/* Drain the `(( ... ))` token stream to gather the expression text for the
   error message. We do not try to re-parse -- we just want to show the user
   what they typed. has_inner_paren is set when a nested `(` was found, which
   changes the error message format to suggest a missing `)`. */
void	handle_arith_error_collect(t_deque_tok *tokens,
							t_string *expr_buf,
							bool *has_inner_paren,
							t_token *last_word)
{
	int		depth;
	t_token	peek;

	depth = 1;
	while (depth > 0)
	{
		peek = *(t_token *)deque_peek(&tokens->deqtok);
		if (peek.tt == TT_END)
			break ;
		if (peek.tt == TT_BRACE_LEFT)
		{
			*has_inner_paren = true;
			append_paren_and_inc(tokens, expr_buf, &depth);
		}
		else if (peek.tt == TT_BRACE_RIGHT)
			handle_right_brace(tokens, expr_buf, &depth);
		else if (peek.tt == TT_WORD)
			collect_word_token(tokens, expr_buf, last_word);
		else
			(void)deque_pop_start(&tokens->deqtok);
	}
}

/* Called when the parser sees a bare TT_ARITH_START (`((`) where a command
   is expected but the context cannot execute arithmetic. We drain the token
   stream to recover the expression text, print a bash-compatible error
   message, then set RES_ERR so the REPL reports a syntax error and discards
   the partial AST. Freeing expr_buf.ctx here is the only allocation cleanup
   needed -- the deque nodes are owned by the tokenizer. */
int	handle_arith_error(t_shell *state,
						t_parser *parser,
						t_deque_tok *tokens,
						t_ast_node *ret)
{
	t_token		arith_tok;
	t_string	expr_buf;
	bool		has_inner_paren;
	t_token		last_word;

	arith_tok = *(t_token *)deque_pop_start(&tokens->deqtok);
	(void)arith_tok;
	vec_init(&expr_buf);
	expr_buf.elem_size = 1;
	has_inner_paren = false;
	last_word = (t_token){0};
	handle_arith_error_collect(tokens, &expr_buf, &has_inner_paren, &last_word);
	handle_arith_error_print(state, has_inner_paren, last_word);
	xfree(expr_buf.ctx);
	parser->res = RES_ERR;
	set_cmd_status(state, res_status(1));
	(void)ret;
	return (1);
}
