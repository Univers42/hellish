/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   parser_proc_sub.c                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 19:10:40 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/26 01:04:12 by dlesieur         ###   ########.fr       */
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

static int	collect_proc_sub_command(t_parser *parser,
								t_deque_tt *tokens,
								const char **cmd_start,
								const char **cmd_end)
{
	int		depth;
	t_token	curr;

	depth = 1;
	*cmd_start = NULL;
	*cmd_end = NULL;
	while (depth > 0)
	{
		curr = *(t_token *)deque_peek(&tokens->deqtok);
		if (curr.tt == TT_END)
		{
			parser->res = RES_MoreInput;
			tokens->looking_for = ')';
			return (1);
		}
		proc_sub_update_depth(curr, &depth);
		if (depth > 0)
		{
			if (!*cmd_start)
				*cmd_start = curr.start;
			*cmd_end = curr.start + curr.len;
		}
		deque_pop_start(&tokens->deqtok);
	}
	return (0);
}

static int	push_cmd_word_node(t_parser *parser,
			const char *cmd_start,
			const char *cmd_end,
			t_ast_node *ret)
{
	char		*cmd_copy;
	size_t		len;
	t_ast_node	word_node;
	t_ast_node	tok_node;

	if (!cmd_start || !cmd_end || cmd_end <= cmd_start)
	{
		parser->res = RES_FatalError;
		return (1);
	}
	len = cmd_end - cmd_start;
	cmd_copy = ft_strndup(cmd_start, len);
	if (!cmd_copy)
	{
		parser->res = RES_FatalError;
		return (1);
	}
	word_node = create_node_type(AST_WORD);
	vec_init(&word_node.children);
	word_node.children.elem_size = sizeof(t_ast_node);
	tok_node = create_node_tok(AST_TOKEN,
			create_tok4(cmd_copy, (int)len, TT_WORD, true));
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
	const char	*cmd_start;
	const char	*cmd_end;

	(void)state;
	ret = create_node_type(AST_PROC_SUB);
	vec_init(&ret.children);
	ret.children.elem_size = sizeof(t_ast_node);
	op_tok = *(t_token *)deque_pop_start(&tokens->deqtok);
	add_op_token_child(&ret, op_tok);
	if (collect_proc_sub_command(parser, tokens, &cmd_start, &cmd_end))
		return (ret);
	if (push_cmd_word_node(parser, cmd_start, cmd_end, &ret))
		return (ret);
	return (ret);
}
