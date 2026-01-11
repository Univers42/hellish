/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   parser_command.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 23:32:16 by marvin            #+#    #+#             */
/*   Updated: 2026/01/09 23:32:16 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "shell.h"

t_ast_node unexpected(t_shell *state, t_parser *parser, t_ast_node ret, t_deque_tt *tokens);

bool	is_redirect(t_tt tt)
{
	if (tt == TT_REDIRECT_LEFT
		|| tt == TT_REDIRECT_RIGHT
		|| tt == TT_APPEND
		|| tt == TT_HEREDOC)
		return (true);
	return (false);
}

t_ast_node	parse_word(t_deque_tt *tokens)
{
	t_ast_node	ret;
	t_token		tmp;

	ret = (t_ast_node){.node_type = AST_WORD};
	vec_init(&ret.children);
	ret.children.elem_size = sizeof(t_ast_node);
	tmp = *(t_token *)deque_pop_start(&tokens->deqtok);
	{
		t_ast_node token_node = (t_ast_node){.node_type = AST_TOKEN, .token = tmp};
		vec_init(&token_node.children);
		token_node.children.elem_size = sizeof(t_ast_node);
		vec_push(&ret.children, &token_node);
	}
	return (ret);
}

t_ast_node	parse_redirect(t_shell *state, t_parser *parser, t_deque_tt *tokens)
{
	t_ast_node	ret;
	t_token		t;

	ret = (t_ast_node){.node_type = AST_REDIRECT};
	vec_init(&ret.children);
	ret.children.elem_size = sizeof(t_ast_node);
	t = *(t_token *)deque_pop_start(&tokens->deqtok);
	if (!is_redirect(t.tt))
		return (unexpected(state, parser, ret, tokens));
	{
		t_ast_node tmp_node = (t_ast_node){.node_type = AST_TOKEN, .token = t};
		vec_init(&tmp_node.children);
		tmp_node.children.elem_size = sizeof(t_ast_node);
		vec_push(&ret.children, &tmp_node);
	}
	/* validate next token is a proper filename/token for redirection */
	{
		t_token next = *(t_token *)deque_peek(&tokens->deqtok);
		if (!(next.tt == TT_WORD || next.tt == TT_SQWORD
			|| next.tt == TT_DQWORD || next.tt == TT_ENVVAR
			|| next.tt == TT_DQENVVAR))
		{
			/* Syntax error: unexpected token after redirection */
			return (unexpected(state, parser, ret, tokens));
		}
	}
	{
		t_ast_node tmp_node = parse_word(tokens);
		vec_push(&ret.children, &tmp_node);
	}
	return (ret);
}

t_ast_node	parse_simple_command(t_shell *state, t_parser *res,
	t_deque_tt *tokens)
{
	t_ast_node	ret;
	t_tt		next;

	ret = (t_ast_node){.node_type = AST_SIMPLE_COMMAND};
	vec_init(&ret.children);
	ret.children.elem_size = sizeof(t_ast_node);
	next = (*(t_token *)deque_peek(&tokens->deqtok)).tt;
	if (!is_simple_cmd_token(next))
		return (unexpected(state, res, ret, tokens));
	while (1)
	{
		next = (*(t_token *)deque_peek(&tokens->deqtok)).tt;
		if (next == TT_WORD)
		{
			t_ast_node tmp_node = parse_word(tokens);
			vec_push(&ret.children, &tmp_node);
		}
		else if (is_redirect(next))
		{
			t_ast_node tmp_node = parse_redirect(state, res, tokens);
			vec_push(&ret.children, &tmp_node);
			if (res->res != RES_OK)
				return (ret);
		}
		else
			break ;
	}
	return (ret);
}

t_ast_node	parse_command(t_shell *state, t_parser *parser, t_deque_tt *tokens)
{
	t_ast_node	ret;
	t_tt		next;

	ret = (t_ast_node){.node_type = AST_COMMAND};
	vec_init(&ret.children);
	ret.children.elem_size = sizeof(t_ast_node);
	next = (*(t_token *)deque_peek(&tokens->deqtok)).tt;
	if (next == TT_BRACE_LEFT)
	{
		{
			t_ast_node tmp_node = parse_subshell(state, parser, tokens);
			vec_push(&ret.children, &tmp_node);
		}
		if (parser->res != RES_OK)
			return (ret);
		while (is_redirect((*(t_token *)deque_peek(&tokens->deqtok)).tt))
		{
			t_ast_node tmp_node = parse_redirect(state, parser, tokens);
			vec_push(&ret.children, &tmp_node);
			if (parser->res != RES_OK)
				return (ret);
		}
	}
	else
	{
		t_ast_node tmp_node = parse_simple_command(state, parser, tokens);
		vec_push(&ret.children, &tmp_node);
		if (parser->res != RES_OK)
			return (ret);
	}
	return (ret);
}
