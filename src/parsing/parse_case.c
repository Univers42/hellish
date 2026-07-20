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

/* Peek at the front of the token deque without consuming it. The alias `pk`
   is used throughout the case parser to keep the frequently repeated
   deque_peek expression short without introducing a macro. */
t_token	*pk(t_deque_tok *t)
{
	return ((t_token *)deque_peek(&t->deqtok));
}

/* True if the token is the word "in" -- used to detect the `case WORD in`
   separator. Like `in` in a for-loop, this is classified as TT_WORD by the
   lexer (it is only a keyword in specific positions), so we compare raw
   text rather than the token type. */
bool	is_kw_in(t_token *tk)
{
	return (tk->tt == TT_WORD && tk->len == 2
		&& ft_strncmp(tk->start, "in", 2) == 0);
}

/* Consume the pattern list of a case item: [(] pattern [| pattern]... ).
   The optional leading `(` is a TT_BRACE_LEFT token; if present we discard
   it. Then we push each pattern word, consuming `|` separators, until the
   closing `)` (TT_BRACE_RIGHT) is found (validated by the caller). */
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

/* Parse one case clause: [(] pattern [| pattern]... ) [compound-list] [;;]
   AST_CASE_ITEM children = pattern word nodes followed by the body compound-
   list. An empty body (pattern followed immediately by `;;` or `esac`) is
   allowed; we create an empty AST_COMPOUND_LIST node for the executor to
   skip cleanly. The trailing `;;` is consumed here so the outer loop in
   parse_case_command sees the next pattern or `esac`. */
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
		ast_push_child(&item, &body);
		if (pk(tokens)->tt == TT_DSEMI)
			(void)deque_pop_start(&tokens->deqtok);
		return (item);
	}
	push_parsed_compound_list(state, parser, tokens, &item);
	if (parser->res == RES_OK && pk(tokens)->tt == TT_DSEMI)
		(void)deque_pop_start(&tokens->deqtok);
	return (item);
}
