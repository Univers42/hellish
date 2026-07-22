/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   utils6.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/03/14 12:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/03/14 12:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "parser_private.h"

/* True when tt can open a compound command: one of the structural keywords
   or `!` (pipeline negation). TT_BANG is included here because `! pipeline`
   is grammatically a command and needs to be routed through the compound path
   rather than falling through to simple-command. TT_LBRACE is for `{ }`;
   TT_ARITH_START is the `((` of an arithmetic command. */
bool	is_compound_start(t_tt tt)
{
	return (tt == TT_IF || tt == TT_WHILE || tt == TT_UNTIL
		|| tt == TT_FOR || tt == TT_CASE || tt == TT_LBRACE
		|| tt == TT_BANG || tt == TT_ARITH_START);
}

/* True when tt is a keyword that closes a compound construct. The parser
   checks this before trying to parse another pipeline inside a compound-list
   so that `do done` and `then fi` are not mistaken for commands. TT_DSEMI
   terminates case clauses. TT_RBRACE closes brace groups. */
bool	is_compound_terminator(t_tt tt)
{
	return (tt == TT_BRACE_RIGHT || tt == TT_THEN || tt == TT_ELIF
		|| tt == TT_ELSE || tt == TT_FI || tt == TT_DO
		|| tt == TT_DONE || tt == TT_ESAC || tt == TT_RBRACE
		|| tt == TT_DSEMI);
}

/* True when the last child of ret was a `;` or newline separator AND the
   next token is a compound terminator. This combination means the body of
   the compound command is complete -- e.g. `while ...; do cmd; done` where
   the `;` before `done` ends the body without starting a new pipeline. */
bool	is_separator_before_terminator(t_ast_node *ret, t_deque_tok *tokens)
{
	size_t	len;
	t_tt	next_tt;
	t_tt	last_tt;

	len = ret->children.len;
	if (len == 0)
		return (false);
	last_tt = ((t_ast_node *)ret->children.ctx)[len - 1].token.tt;
	next_tt = (*(t_ltoken *)deque_peek(&tokens->deqtok)).tt;
	return ((last_tt == TT_SEMICOLON || last_tt == TT_NEWLINE)
		&& is_compound_terminator(next_tt));
}

/* Consume all leading TT_NEWLINE tokens from the deque. Called at the start
   of compound-command body parsing and before expecting closing keywords so
   that multi-line commands like `if true\nthen\necho hi\nfi` work cleanly. */
void	skip_newlines(t_deque_tok *tokens)
{
	while ((*(t_ltoken *)deque_peek(&tokens->deqtok)).tt == TT_NEWLINE)
		(void)deque_pop_start(&tokens->deqtok);
}
