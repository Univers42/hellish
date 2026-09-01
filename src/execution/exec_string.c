/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   exec_string.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/02 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/02 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "execution_private.h"
#include "lexer.h"
#include "parser.h"
#include "decomposer.h"
#include "redir.h"

void	report_parse_error(t_shell *state, t_parser *parser, t_deque_tok *tt);

/* Parse + execute a string one statement at a time. Used by eval, command
   and the dot/source builtin -- since issue #105 the string reaches the
   parser in hazard-clipped chunks (exec_string3.c) so statements that
   change how later text lexes take effect for the rest of the string;
   run_one_stmt and skip_delimiters below are shared with that chunker's
   error replay. `str` must stay valid for the whole call. */
/* Drop any leading newline/semicolon tokens from the token queue before
   the next statement is parsed.  exec_string feeds multiple statements
   from a single token stream, so after each statement the queue may have
   stale statement-separators that would confuse the next parse call. */
void	skip_delimiters(t_deque_tok *tt)
{
	t_tt	t;

	t = ((t_ltoken *)deque_peek(&tt->deqtok))->tt;
	while (t == TT_NEWLINE || t == TT_SEMICOLON)
	{
		(void)deque_pop_start(&tt->deqtok);
		t = ((t_ltoken *)deque_peek(&tt->deqtok))->tt;
	}
}

/* Parse and execute one statement from the token queue.  On a parse error
   we set $? to 2 (syntax error) and signal stop so the caller breaks out
   of the loop.  The AST is freed unconditionally -- it is a per-statement
   allocation and must not escape this function.  *stop is also set when
   the shell hits an unconditional-exit condition (break outside loop,
   return outside function, exit, SIGTERM unwind).

   RES_GETMOREINPUT means an unterminated construct with no more input to
   come, and it has to be REPORTED here.  stream_finish() does it for the
   main input path (input_stream.c), but eval and source come through this
   function instead -- so `source file` on a file ending mid-`{` set $? to 2
   and printed nothing at all.  A file that fails to load and says nothing
   about it is the worst possible answer, and it is what every plugin with a
   brace hellish could not parse was getting.  A plain RES_ERR already
   printed its own message during the parse. */
int	run_one_stmt(t_shell *state, t_deque_tok *tt, bool *stop)
{
	t_parser	parser;
	t_ast_node	ast;
	int			status;

	parser = (t_parser){.res = RES_OK};
	vec_init(&parser.parse_stack);
	parser.parse_stack.elem_size = sizeof(int);
	ast = parse_simple_list(state, &parser, tt);
	status = 2;
	if (parser.res == RES_OK)
		status = run_parsed(state, &ast);
	else
		report_parse_error(state, &parser, tt);
	free_ast(&ast);
	xfree(parser.parse_stack.ctx);
	*stop = (parser.res != RES_OK || must_stop(state));
	if (!*stop)
		skip_delimiters(tt);
	return (status);
}

/* Extract heredoc bodies up front (so they aren't parsed as commands),
   feed them to the heredoc reader via state->hd_src, and run the stripped
   text.  A string without << (or where splitting finds nothing) runs
   as-is, under whatever hd_src the caller already installed. Stripping
   happens on the RAW text now that alias splicing is per-chunk (#105):
   bodies are removed before any splice, so alias words inside a heredoc
   body are never expanded -- which is also what bash does. */
static int	exec_split_heredocs(t_shell *state, char *str, char **bodies)
{
	char	*stripped;
	int		status;

	stripped = NULL;
	if (ft_strnstr(str, "<<", ft_strlen(str))
		&& split_heredocs(str, &stripped, bodies))
	{
		state->hd_src = *bodies;
		state->hd_pos = 0;
		status = exec_chunks(state, stripped);
		xfree(stripped);
	}
	else
		status = exec_chunks(state, str);
	return (status);
}

/* Execute the string, routing any heredoc bodies aside first.  Alias
   expansion happens inside exec_chunks, one chunk at a time, so an alias
   (or shopt, or dialect) set early in the string shapes the rest of it.
   hd_src/hd_pos are saved/restored so nested command substitutions each
   get their own body stream.

   The private copy is load-bearing, not defensive fluff: callers pass
   SHELL STATE as the string -- open_cycle hands over $PROMPT_COMMAND's
   own env buffer -- and a statement inside can rewrite that variable
   (bash-preexec's __bp_install does exactly this), freeing the buffer
   mid-run. The chunker re-reads the source text after every executed
   statement, so without the copy that free is a use-after-free the
   fresh-install pty test catches under ASan. The old single-pass code
   was immune only by accident: its up-front alias splice WAS the copy. */
int	exec_string(t_shell *state, char *str)
{
	char	*bodies;
	char	*own;
	char	*prev_src;
	size_t	prev_pos;
	int		status;

	prev_src = state->hd_src;
	prev_pos = state->hd_pos;
	bodies = NULL;
	own = ft_strdup(str);
	status = exec_split_heredocs(state, own, &bodies);
	xfree(own);
	xfree(bodies);
	state->hd_src = prev_src;
	state->hd_pos = prev_pos;
	return (status);
}
