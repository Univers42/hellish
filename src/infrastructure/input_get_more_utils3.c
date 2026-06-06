/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   input_get_more_utils3.c                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 17:47:07 by dlesieur          #+#    #+#             */
/*   Updated: 2026/03/14 14:07:05 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "input_private.h"

/* Convenience wrapper: dump the accumulated token list and reset for the next
   command. Used by both the debug-lexer and the debug-parser loops after each
   input round. */
void	print_and_cleanup_tokens(t_shell *state,
									t_deque_tok *tt,
									char **prompt)
{
	debug_lexer_print_tokens(state, tt);
	debug_lexer_cleanup(state, tt, prompt);
}

/* Fetch more tokens and handle EOF/interrupt with one retry. Returns 1 (break
   the outer loop), 2 (continue — the interrupt was handled, try again) or 0
   (tokens are ready to parse). The second attempt after an interrupt lets the
   debug-parser loop survive a stray ^C without losing the whole session. */
static int	fetch_and_handle_input(t_shell *state,
									t_parser *parser,
									char **prompt,
									t_deque_tok *tt)
{
	int	s;

	(void)parser;
	s = get_more_tokens(state, prompt, tt);
	if (handle_eof(s, state))
		return (1);
	if (handle_interrupt(s, state, tt, prompt) == 2)
	{
		s = get_more_tokens(state, prompt, tt);
		if (handle_eof(s, state))
			return (1);
		if (handle_interrupt(s, state, tt, prompt) == 2)
			return (2);
	}
	return (0);
}

/* Parse the current token list, emit the AST as a Graphviz DOT file, clean up,
   and reset the parser to RES_INIT so the debug-parser loop can accept the
   next command without restarting. reclassify_keywords re-evaluates ambiguous
   tokens (e.g. "do" that the lexer emitted as a word) before the full parse. */
static void	parse_print_and_cleanup(t_shell *state,
									t_parser *parser,
									t_deque_tok *tt,
									char **prompt)
{
	t_ast_node	parsed;

	parser->parse_stack.len = 0;
	reclassify_keywords(tt);
	parsed = parse_tokens(state, parser, tt);
	debug_parser_print_ast(state, parser, parsed);
	debug_parser_cleanup(state, tt, prompt);
	parser->res = RES_INIT;
}

/* Debug-parser REPL: like default_parser_loop but instead of executing the
   tree we write out.dot and continue. Lets you visually inspect the AST for
   each command while the shell stays alive. Activated by --debug-parser. */
void	debug_parser_loop(t_shell *state, t_parser *parser,
							char **prompt, t_deque_tok *tt)
{
	int	action;

	while (parser->res == RES_GETMOREINPUT || parser->res == RES_INIT)
	{
		set_cmd_status(state, res_status(0));
		action = fetch_and_handle_input(state, parser, prompt, tt);
		if (action == 1)
			break ;
		if (action == 2)
			continue ;
		set_cmd_status(state, res_status(0));
		if (get_g_sig()->should_unwind)
			set_cmd_status(state, create_exec_state(CANCELED, true));
		if (state->should_exit || get_g_sig()->should_unwind)
			break ;
		if (is_empty_token_list(tt))
		{
			buff_readline_reset(&state->rl);
			continue ;
		}
		parse_print_and_cleanup(state, parser, tt, prompt);
	}
}
