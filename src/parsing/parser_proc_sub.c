/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   parser_proc_sub.c                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 19:10:40 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/20 19:10:43 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "parser_private.h"


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
