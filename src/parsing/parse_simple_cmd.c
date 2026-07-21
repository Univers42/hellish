/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   parse_simple_cmd.c                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 19:10:59 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/27 16:18:59 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "parser_private.h"

/* Consume one element of a simple command: a word argument, a redirect, or
   a process substitution `<(...)` / `>(...)`. Returns true if something was
   consumed, false when the next token ends the command (e.g. `;`, `|`, `&&`,
   or any keyword). Returning false is normal termination for the scanner
   loop, not an error. */
static bool	parse_and_push_simple_cmd_child(t_shell *state,
											t_parser *res,
											t_deque_tok *tokens,
											t_ast_node *ret)
{
	t_tt	next;

	next = (*(t_token *)deque_peek(&tokens->deqtok)).tt;
	if (next == TT_WORD && try_push_array_assign(res, tokens, ret))
		return (check_res_ok(res));
	if (next == TT_WORD)
		return (push_parsed_word(tokens, ret), true);
	else if (is_redirect(next))
		return (push_parsed_redirect(state, res, tokens, ret),
			check_res_ok(res));
	else if (is_proc_sub(next))
		return (push_parsed_proc_sub(state, res, tokens, ret),
			check_res_ok(res));
	return (false);
}

/* Parse a simple command: word [redirect|word|procsub]...
   We scan greedily until parse_and_push_simple_cmd_child returns false.
   The initial is_simple_cmd_token check protects against being called on a
   keyword or operator -- if it fails we hand off to the unexpected-token
   handler rather than producing an empty AST node. ST_SCANNING is a
   convenience macro that always evaluates to 1 (loop forever; break inside
   is the exit). */
t_ast_node	parse_simple_command(t_shell *state, t_parser *res,
								t_deque_tok *tokens)
{
	t_ast_node	ret;
	t_tt		next;

	ret = (t_ast_node){.node_type = AST_SIMPLE_COMMAND};
	vec_init(&ret.children);
	ret.children.elem_size = sizeof(t_ast_node);
	next = (*(t_token *)deque_peek(&tokens->deqtok)).tt;
	if (!is_simple_cmd_token(next))
		return (unexpected(state, res, ret, tokens));
	while (ST_SCANNING)
		if (!parse_and_push_simple_cmd_child(state, res, tokens, &ret))
			break ;
	return (ret);
}
