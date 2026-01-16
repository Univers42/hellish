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
#include "lexer.h"
#include "parser.h"

t_ast_node	unexpected(t_shell *state, t_parser *parser,
				t_ast_node ret, t_deque_tt *tokens);

bool	is_redirect(t_tt tt)
{
	if (tt == TT_REDIRECT_LEFT
		|| tt == TT_REDIRECT_RIGHT
		|| tt == TT_APPEND
		|| tt == TT_HEREDOC)
		return (true);
	return (false);
}

bool	is_proc_sub(t_tt tt)
{
	return (tt == TT_PROC_SUB_IN || tt == TT_PROC_SUB_OUT);
}

bool	is_simple_cmd_token(t_tt tt)
{
	return (tt == TT_WORD || is_redirect(tt) || is_proc_sub(tt));
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
			return (unexpected(state, parser, ret, tokens));
		}
	}
	{
		t_ast_node tmp_node = parse_word(tokens);
		vec_push(&ret.children, &tmp_node);
	}
	return (ret);
}

/* Parse process substitution: <(command) or >(command) */
t_ast_node	parse_proc_sub(t_shell *state, t_parser *parser, t_deque_tt *tokens)
{
	t_ast_node	ret;
	t_token		op_tok;
	int			depth;
	t_string	cmd_str;
	t_token		curr;

	(void)state;
	ret = (t_ast_node){.node_type = AST_PROC_SUB};
	vec_init(&ret.children);
	ret.children.elem_size = sizeof(t_ast_node);

	/* Pop the <( or >( operator token */
	op_tok = *(t_token *)deque_pop_start(&tokens->deqtok);
	{
		t_ast_node op_node = (t_ast_node){.node_type = AST_TOKEN, .token = op_tok};
		vec_init(&op_node.children);
		op_node.children.elem_size = sizeof(t_ast_node);
		vec_push(&ret.children, &op_node);
	}

	/* Collect tokens until matching ) */
	vec_init(&cmd_str);
	cmd_str.elem_size = 1;
	depth = 1;

	while (depth > 0)
	{
		curr = *(t_token *)deque_peek(&tokens->deqtok);
		if (curr.tt == TT_END)
		{
			parser->res = RES_MoreInput;
			tokens->looking_for = ')';
			free(cmd_str.ctx);
			return (ret);
		}
		if (curr.tt == TT_BRACE_LEFT || curr.tt == TT_PROC_SUB_IN
			|| curr.tt == TT_PROC_SUB_OUT)
			depth++;
		else if (curr.tt == TT_BRACE_RIGHT)
			depth--;
		if (depth > 0)
		{
			/* Append token text to command string */
			deque_pop_start(&tokens->deqtok);
			if (cmd_str.len > 0)
			{
				char sp = ' ';
				vec_push(&cmd_str, &sp);
			}
			vec_push_nstr(&cmd_str, curr.start, curr.len);
		}
		else
		{
			/* Consume the closing ) */
			deque_pop_start(&tokens->deqtok);
		}
	}

	/* Create a WORD node containing the command string */
	{
		char		*cmd_copy;
		t_ast_node	word_node;
		t_ast_node	tok_node;

		if (!vec_ensure_space_n(&cmd_str, 1))
		{
			parser->res = RES_FatalError;
			free(cmd_str.ctx);
			return (ret);
		}
		((char *)cmd_str.ctx)[cmd_str.len] = '\0';
		cmd_copy = ft_strdup((char *)cmd_str.ctx);
		free(cmd_str.ctx);
		word_node = (t_ast_node){.node_type = AST_WORD};
		vec_init(&word_node.children);
		word_node.children.elem_size = sizeof(t_ast_node);
		tok_node = (t_ast_node){.node_type = AST_TOKEN};
		tok_node.token = create_tok4(cmd_copy, (int)ft_strlen(cmd_copy), TT_WORD, true);
		vec_init(&tok_node.children);
		tok_node.children.elem_size = sizeof(t_ast_node);
		vec_push(&word_node.children, &tok_node);
		vec_push(&ret.children, &word_node);
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
		else if (is_proc_sub(next))
		{
			t_ast_node tmp_node = parse_proc_sub(state, res, tokens);
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

	/* Check for arithmetic expression (( which is not supported */
	if (next == TT_ARITH_START)
	{
		parser->res = RES_FatalError;
		state->last_cmd_status_res = res_status(1);
		return (ret);
	}

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
