/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   parser_proc_sub.c                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 19:10:40 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/26 00:37:29 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "parser_private.h"

static void	proc_sub_update_depth(t_token curr, int *depth)
{
	if (curr.tt == TT_BRACE_LEFT
		|| curr.tt == TT_PROC_SUB_IN
		|| curr.tt == TT_PROC_SUB_OUT)
		(*depth)++;
	else if (curr.tt == TT_BRACE_RIGHT)
		(*depth)--;
}

static void	proc_sub_consume_and_append(t_deque_tt *tokens,
												t_string *cmd_str,
												t_token curr)
{
	deque_pop_start(&tokens->deqtok);
	if (cmd_str->len > 0)
		vec_push_char(cmd_str, ' ');
	vec_push_nstr(cmd_str, curr.start, curr.len);
}

/* Collect tokens inside process-substitution until matching ')' is found.
   On EOF sets parser->res = RES_MoreInput, tokens->looking_for = ')' and
   frees cmd_str.ctx; returns 1 on early EOF, 0 on success. */
static int	collect_proc_sub_command(t_parser *parser,
								t_deque_tt *tokens,
								t_string *cmd_str)
{
	int		depth;
	t_token	curr;

	depth = 1;
	while (depth > 0)
	{
		curr = *(t_token *)deque_peek(&tokens->deqtok);
		if (curr.tt == TT_END)
		{
			parser->res = RES_MoreInput;
			tokens->looking_for = ')';
			free(cmd_str->ctx);
			cmd_str->ctx = NULL;
			return (1);
		}
		proc_sub_update_depth(curr, &depth);
		if (depth > 0)
			proc_sub_consume_and_append(tokens, cmd_str, curr);
		else
			(void)deque_pop_start(&tokens->deqtok);
	}
	return (0);
}

/* Create a WORD node from cmd_str and append to ret.
   Returns 0 on success, 1 on fatal error (parser->res set). */
static int	push_cmd_word_node(t_parser *parser,
			t_string *cmd_str,
			t_ast_node *ret)
{
	char		*cmd_copy;
	t_ast_node	word_node;
	t_ast_node	tok_node;

	if (!vec_ensure_space_n(cmd_str, 1))
	{
		parser->res = RES_FatalError;
		free(cmd_str->ctx);
		return (1);
	}
	((char *)cmd_str->ctx)[cmd_str->len] = '\0';
	cmd_copy = ft_strdup((char *)cmd_str->ctx);
	free(cmd_str->ctx);
	cmd_str->ctx = NULL;
	word_node = create_node_type(AST_WORD);
	vec_init(&word_node.children);
	word_node.children.elem_size = sizeof(t_ast_node);
	tok_node = create_node_tok(AST_TOKEN,
			create_tok4(cmd_copy, (int)ft_strlen(cmd_copy), TT_WORD, true));
	vec_init(&tok_node.children);
	tok_node.children.elem_size = sizeof(t_ast_node);
	vec_push(&word_node.children, &tok_node);
	vec_push(&ret->children, &word_node);
	return (0);
}

t_ast_node	parse_proc_sub(t_shell *state, t_parser *parser, t_deque_tt *tokens)
{
	t_ast_node	ret;
	t_token		op_tok;
	t_string	cmd_str;

	(void)state;
	ret = create_node_type(AST_PROC_SUB);
	vec_init(&ret.children);
	ret.children.elem_size = sizeof(t_ast_node);
	op_tok = *(t_token *)deque_pop_start(&tokens->deqtok);
	add_op_token_child(&ret, op_tok);
	vec_init(&cmd_str);
	cmd_str.elem_size = 1;
	if (collect_proc_sub_command(parser, tokens, &cmd_str))
		return (ret);
	if (push_cmd_word_node(parser, &cmd_str, &ret))
		return (ret);
	return (ret);
}
