/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   parse_case.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/02 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/02 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "parser_private.h"

t_token	*pk(t_deque_tok *t)
{
	return ((t_token *)deque_peek(&t->deqtok));
}

bool	is_kw_in(t_token *tk)
{
	return (tk->tt == TT_WORD && tk->len == 2
		&& ft_strncmp(tk->start, "in", 2) == 0);
}

static void	parse_case_patterns(t_deque_tok *tokens, t_ast_node *item)
{
	if (pk(tokens)->tt == TT_BRACE_LEFT)
		(void)deque_pop_start(&tokens->deqtok);
	push_parsed_word(tokens, item);
	while (pk(tokens)->tt == TT_PIPE)
	{
		(void)deque_pop_start(&tokens->deqtok);
		push_parsed_word(tokens, item);
	}
}

/* Parse one clause: [(] pattern [| pattern]... ) [compound-list] [;;]
   AST_CASE_ITEM children = pattern words... + trailing compound-list body. */
t_ast_node	parse_case_item(t_shell *state, t_parser *parser,
					t_deque_tok *tokens)
{
	t_ast_node	item;
	t_ast_node	body;

	init_ast_node_children(&item, AST_CASE_ITEM);
	parse_case_patterns(tokens, &item);
	if (pk(tokens)->tt != TT_BRACE_RIGHT)
		return (parser->res = RES_ERR, item);
	(void)deque_pop_start(&tokens->deqtok);
	skip_newlines(tokens);
	if (pk(tokens)->tt == TT_DSEMI || pk(tokens)->tt == TT_ESAC)
	{
		init_ast_node_children(&body, AST_COMPOUND_LIST);
		vec_push(&item.children, &body);
		if (pk(tokens)->tt == TT_DSEMI)
			(void)deque_pop_start(&tokens->deqtok);
		return (item);
	}
	push_parsed_compound_list(state, parser, tokens, &item);
	if (parser->res == RES_OK && pk(tokens)->tt == TT_DSEMI)
		(void)deque_pop_start(&tokens->deqtok);
	return (item);
}
