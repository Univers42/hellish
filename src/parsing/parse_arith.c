/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   parse_arith.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 19:52:24 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/27 16:07:19 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "parser_private.h"

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
	free(expr_buf.ctx);
	parser->res = RES_FatalError;
	set_cmd_status(state, res_status(1));
	(void)ret;
	return (1);
}
