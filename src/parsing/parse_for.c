/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   parse_for.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/03/14 12:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/03/14 12:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "parser_private.h"

/* True for any of the five token types that represent a shell word (bare,
   single-quoted, double-quoted, env-var). Used to validate the for-variable
   name and to collect the optional `in wordlist` items. */
bool	is_for_word(t_tt tt)
{
	return (tt == TT_WORD || tt == TT_SQWORD || tt == TT_DQWORD
		|| tt == TT_ENVVAR || tt == TT_DQENVVAR);
}

/* Consume consecutive word tokens and push each as a child of ret. This
   is the optional word-list after `in` in `for x in a b c; do ...`. We
   stop on the first non-word token (`;`, newline, `do`, etc.). */
static void	collect_word_list(t_deque_tok *tokens, t_ast_node *ret)
{
	t_tt	next;

	next = (*(t_ltoken *)deque_peek(&tokens->deqtok)).tt;
	while (is_for_word(next))
	{
		push_parsed_word(tokens, ret);
		next = (*(t_ltoken *)deque_peek(&tokens->deqtok)).tt;
	}
}

/* Handle the optional `in wordlist [;|\n]` clause. If the next word is
   literally "in" we set ret->negate (repurposed as "has_in_clause" flag),
   collect the word list, and consume the trailing separator. If there is no
   `in` clause (just `;` or newline), we iterate over `"$@"` at runtime.
   Gotcha: `in` is not a keyword token -- the lexer leaves it as TT_WORD and
   we compare the raw text here because the for-variable name already consumed
   the keyword-position slot. */
bool	for_in_clause(t_shell *state, t_parser *parser,
			t_deque_tok *tokens, t_ast_node *ret)
{
	t_tt		next;
	t_ltoken	*peek;

	(void)state;
	skip_newlines(tokens);
	next = (*(t_ltoken *)deque_peek(&tokens->deqtok)).tt;
	if (next == TT_END)
		return (parser->res = RES_GETMOREINPUT, false);
	peek = (t_ltoken *)deque_peek(&tokens->deqtok);
	if (next == TT_WORD && peek->len == 2
		&& kw_eq(tokens->base + peek->off, "in", 2))
	{
		ret->negate = true;
		(void)deque_pop_start(&tokens->deqtok);
		collect_word_list(tokens, ret);
		next = (*(t_ltoken *)deque_peek(&tokens->deqtok)).tt;
		if (next == TT_SEMICOLON || next == TT_NEWLINE)
			(void)deque_pop_start(&tokens->deqtok);
	}
	else if (next == TT_SEMICOLON || next == TT_NEWLINE)
		(void)deque_pop_start(&tokens->deqtok);
	return (true);
}

/* Parse: for NAME [in wordlist [;|\n]] do compound_list done
   AST_FOR: ret.token = the NAME word, children = optional word items from
   the in-clause followed by the body compound_list. The body always comes
   last so the executor can distinguish words from the body by position.
   Everything after the keyword is parse_for_tail (parse_select.c), which
   `select` shares: the two differ only in node type and in what the
   executor does with the words. */
t_ast_node	parse_for_command(t_shell *state, t_parser *parser,
							t_deque_tok *tokens)
{
	t_ast_node	ret;
	t_tt		next;

	init_ast_node_children(&ret, AST_FOR);
	(void)deque_pop_start(&tokens->deqtok);
	next = (*(t_ltoken *)deque_peek(&tokens->deqtok)).tt;
	if (next == TT_ARITH_START)
		return (free_ast(&ret), parse_for_arith(state, parser, tokens));
	vec_push_int(&parser->parse_stack, TT_FOR);
	return (parse_for_tail(state, parser, tokens, ret));
}
