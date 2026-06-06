/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   input_get_more_utils.c                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 17:33:06 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/27 16:23:04 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "input_private.h"

/* Reset exit status to zero and rebuild the prompt for the next command. Used
   after a Ctrl-C cancel or after a debug-mode cleanup: we always want to come
   back to a clean "ready" state so the next read does not inherit stale status.
*/
void	reset_status_and_prompt(t_shell *state, char **prompt)
{
	t_string	p;

	set_cmd_status(state, res_status(0));
	if (*prompt)
		xfree(*prompt);
	if (state->metinp == INP_RL)
	{
		p = prompt_normal(state);
		*prompt = ft_strdup(p.ctx);
		xfree(p.ctx);
	}
	else
		*prompt = ft_strdup("");
}

/* Ctrl-C mid-command: discard everything accumulated so far and repaint a
   clean prompt. Exit status 130 (128 + SIGINT) is the POSIX convention bash
   uses; ctrl_c=true lets wait-builtin and $? distinguish Ctrl-C from a real
   exit 130. We also clear looking_for so the tokenizer does not think it is
   still hunting for a closing quote or fi. */
void	handle_ctrl_c(t_shell *state, t_deque_tok *tt, char **prompt)
{
	buff_readline_reset(&state->rl);
	if (tt->deqtok.buff)
		deque_clear(&tt->deqtok, NULL);
	tt->looking_for = 0;
	if (state->input.ctx)
		state->input.len = 0;
	set_cmd_status(state, (t_execution_state){.status = 130, .ctrl_c = true});
	reset_status_and_prompt(state, prompt);
	buff_readline_update(&state->rl);
}

/* On EOF (stat == 1), set should_exit and return 1. Thin wrapper so the parse
   loops can check the condition with a single call. */
int	handle_eof(int s, t_shell *state)
{
	if (s == 1)
	{
		state->should_exit = true;
		return (1);
	}
	return (0);
}

/* On interrupt (stat == 2, i.e. SIGINT from readline), run the full Ctrl-C
   cleanup and return 2 so the parse loop can decide whether to continue or
   break (interactive mode continues; non-interactive may exit). */
int	handle_interrupt(int s, t_shell *state, t_deque_tok *tt, char **prompt)
{
	if (s == 2)
	{
		handle_ctrl_c(state, tt, prompt);
		return (2);
	}
	return (0);
}

/* Debug-parser mode: emit a Graphviz DOT file for the parsed AST and free it.
   On parse error we still set SYNTAX_ERR so the debug loop can keep running
   without the shell exiting -- good for systematically testing error inputs. */
void	debug_parser_print_ast(t_shell *state,
								t_parser *parser,
								t_ast_node parsed)
{
	if (parser->res == RES_OK || parser->res == RES_GETMOREINPUT)
		print_ast_dot(state, parsed);
	if (parser->res == RES_OK || parser->res == RES_ERR
		|| parser->res == RES_GETMOREINPUT)
		free_ast(&parsed);
	if (parser->res == RES_ERR)
		set_cmd_status(state, (t_execution_state){.status = SYNTAX_ERR});
}
