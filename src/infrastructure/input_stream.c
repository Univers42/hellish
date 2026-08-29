/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   input_stream.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/21 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/07/21 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "input_private.h"
#include "sh_error.h"
#include "parena.h"

/* Streamed execution of a fully-buffered batch: parse ONE newline-delimited
   top-level range, run it, reclaim it, repeat — instead of materialising a
   50k-line AST before the first command runs. This is how bash reads a
   script, and it is what caps our RSS at one range (a few KB) instead of
   one file (~160MB was measured, most of the wall-clock system time).
   It is also more bash-correct on one point: `echo a` in a script whose
   NEXT line opens an unterminated `if` runs before the unexpected-EOF
   error — the whole-batch parser could never execute the healthy prefix. */

/* Stream only when re-reading more input is provably impossible, so a
   mid-stream GETMOREINPUT can never trigger the read-again/reparse-again
   path (which would re-execute already-run ranges): the reader ring must
   be drained, and the source either fully preloaded (-c and script files)
   or at observed EOF (pipes). Interactive input never streams. Heredoc
   cycles are excluded conservatively for now — the body cursor logic is
   order-compatible but unproven under stream teardown. */
static bool	stream_eligible(t_shell *state)
{
	if (state->metinp == INP_RL || state->cycle_has_hd)
		return (false);
	if (state->rl.has_line || state->rl.cursor < state->rl.buff.len)
		return (false);
	if (state->metinp == INP_ARG || state->metinp == INP_FILE)
		return (true);
	return (state->rl.has_finished);
}

/* Post-loop error routing. GETMOREINPUT here means an unterminated
   construct with no more input to come — bash's "unexpected end of file",
   status 2. A plain syntax error already printed its message during the
   parse; both cases end a non-interactive shell (we are never interactive
   on this path). No replay: ranges before the error have already run,
   which is exactly what the line-exact replay used to reconstruct. */
static void	stream_finish(t_shell *state, t_parser *parser, t_deque_tok *tt)
{
	if (parser->res == RES_GETMOREINPUT)
	{
		ft_eprintf("%s: syntax error: unexpected end of file\n",
			state->ctx);
		parser->res = RES_ERR;
	}
	else if (parser->res == RES_ERR && !parser->reported)
		unexpected(state, parser, (t_ast_node){0}, tt);
	if (parser->res == RES_ERR)
	{
		set_cmd_status(state, (t_execution_state){.status = SYNTAX_ERR});
		state->should_exit = true;
	}
}

/* The range loop. Each iteration parses one range under the arena gate,
   executes it, then reclaims: arena-pure trees are dropped wholesale
   (parena_reset), attached ones walk first. Redirects are freed per range
   exactly as repl_shell frees them per cycle. The loop stops on stream
   exhaustion, exit/errexit, SIGINT unwind, or a parse error. */
bool	stream_try(t_shell *state, t_parser *parser, t_deque_tok *tt)
{
	bool	more;

	if (!stream_eligible(state))
		return (false);
	state->cycle_streamed = true;
	more = true;
	while (more && !state->should_exit && !get_g_sig()->should_unwind)
	{
		parser->parse_stack.len = 0;
		parser->stream = 1;
		parena_on(true);
		state->tree = parse_tokens_range(state, parser, tt, &more);
		parena_on(false);
		parser->stream = 0;
		if (parser->res != RES_OK)
			break ;
		execute_top_level(state);
		if (parena()->attached)
			free_ast(&state->tree);
		else
			state->tree = (t_ast_node){0};
		free_redirects(&state->redirects);
		parena_reset();
	}
	return (stream_finish(state, parser, tt), true);
}
