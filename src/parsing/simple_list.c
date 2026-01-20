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

// 0 continue
// 1 break
// 2 return on fail
int	parse_simple_list_s(t_shell *state, t_parser *parser,
	t_deque_tt *tokens, t_ast_node *ret)
{
	t_tt		next;
	t_token		tmp;

	tmp = *(t_token *)deque_peek(&tokens->deqtok);
	next = tmp.tt;
	if (!is_simple_list_op(next))
		return (1);
	{ int val = next; vec_push(&parser->parse_stack, &val); }
	tmp = *(t_token *)deque_pop_start(&tokens->deqtok);
	{
		t_ast_node tmp_node = (t_ast_node){.node_type = AST_TOKEN, .token = tmp};
		vec_init(&tmp_node.children);
		tmp_node.children.elem_size = sizeof(t_ast_node);
		vec_push(&ret->children, &tmp_node);
	}
	if (parser->res != RES_OK)
		return (2);
	while ((*(t_token *)deque_peek(&tokens->deqtok)).tt == TT_NEWLINE)
		(void)deque_pop_start(&tokens->deqtok);
	/* For & and ;, if we hit END or NEWLINE, that's valid - just stop parsing */
	if ((next == TT_SEMICOLON || next == TT_AMPERSAND) 
		&& (*(t_token *)deque_peek(&tokens->deqtok)).tt == TT_END)
		return (2);
	if ((*(t_token *)deque_peek(&tokens->deqtok)).tt == TT_END)
		return (parser->res = RES_MoreInput, 2);
	{
		t_ast_node tmp_node = parse_pipeline(state, parser, tokens);
		vec_push(&ret->children, &tmp_node);
	}
	if (parser->res != RES_OK)
		return (2);
	vec_pop(&parser->parse_stack);
	return (0);
}

t_ast_node	parse_simple_list(t_shell *state,
	t_parser *parser, t_deque_tt *tokens)
{
	t_ast_node	ret;
	t_tt		next;
	int			status;

	ret = (t_ast_node){.node_type = AST_SIMPLE_LIST};
	vec_init(&ret.children);
	ret.children.elem_size = sizeof(t_ast_node);
	next = (*(t_token *)deque_peek(&tokens->deqtok)).tt;
	
	/* Check for arithmetic expression (( at the start */
	if (next == TT_ARITH_START)
	{
		t_token arith_tok = *(t_token *)deque_pop_start(&tokens->deqtok);
		(void)arith_tok;
		
		/* Collect tokens until we find )) or run out */
		t_string expr_buf;
		vec_init(&expr_buf);
		expr_buf.elem_size = 1;
		
		int depth = 1; /* We've consumed ((, so depth starts at 1 for the outer (( */
		t_token last_word = {0};
		bool has_inner_paren = false;
		
		while (depth > 0)
		{
			t_token peek = *(t_token *)deque_peek(&tokens->deqtok);
			if (peek.tt == TT_END)
				break;
			
			if (peek.tt == TT_BRACE_LEFT)
			{
				has_inner_paren = true;
				depth++;
				(void)deque_pop_start(&tokens->deqtok);
				vec_push_str(&expr_buf, "(");
			}
			else if (peek.tt == TT_BRACE_RIGHT)
			{
				depth--;
				if (depth > 0)
				{
					(void)deque_pop_start(&tokens->deqtok);
					vec_push_str(&expr_buf, ")");
				}
				else
				{
					/* This is the first ) of )) - consume both */
					(void)deque_pop_start(&tokens->deqtok);
					peek = *(t_token *)deque_peek(&tokens->deqtok);
					if (peek.tt == TT_BRACE_RIGHT)
					{
						(void)deque_pop_start(&tokens->deqtok);
						depth = 0;
					}
					else
					{
						/* Only one ), not )) */
						depth = 0;
					}
				}
			}
			else if (peek.tt == TT_WORD)
			{
				last_word = peek;
				(void)deque_pop_start(&tokens->deqtok);
				if (expr_buf.len > 0)
				{
					char sp = ' ';
					vec_push(&expr_buf, &sp);
				}
				vec_push_nstr(&expr_buf, peek.start, peek.len);
			}
			else
			{
				(void)deque_pop_start(&tokens->deqtok);
			}
		}
		
		/* Generate error message like bash */
		ft_eprintf("%s: ((: ", state->context);
		if (has_inner_paren && last_word.start)
		{
			/* Like bash: missing `)' (error token is "...") */
			ft_eprintf("missing `)' (error token is \"%.*s)\")\n",
				last_word.len, last_word.start);
		}
		else if (last_word.start)
		{
			/* Like bash: syntax error in expression (error token is "...") */
			ft_eprintf("syntax error in expression (error token is \"%.*s\")\n",
				last_word.len, last_word.start);
		}
		else
		{
			ft_eprintf("syntax error in arithmetic expression\n");
		}
		
		free(expr_buf.ctx);
		parser->res = RES_FatalError;
		/* Set exit code to 1 for arithmetic errors (bash uses 1, not 2) */
		set_cmd_status(state, res_status(1));
		return (ret);
	}
	
	if (!is_simple_cmd_token(next) && next != TT_BRACE_LEFT)
		return (unexpected(state, parser, ret, tokens));
	{
		t_ast_node tmp_node = parse_pipeline(state, parser, tokens);
		vec_push(&ret.children, &tmp_node);
	}
	if (parser->res != RES_OK)
		return (ret);
	while (1)
	{
		status = parse_simple_list_s(state, parser, tokens, &ret);
		if (status == 1)
			break ;
		if (status == 2)
			return (ret);
	}
	if ((*(t_token *)deque_peek(&tokens->deqtok)).tt == TT_NEWLINE)
		(void)deque_pop_start(&tokens->deqtok);
	else if ((*(t_token *)deque_peek(&tokens->deqtok)).tt != TT_END)
		return (unexpected(state, parser, ret, tokens));
	return (ret);
}
