/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   exec_string_err.c                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/02 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/09/03 12:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "execution_private.h"
#include "parser.h"
#include "sh_error.h"

/* Say something.  About a dozen parser productions set RES_ERR without a
   message of their own -- parse_for, parse_if, parse_while, parse_case,
   parse_array, parse_arith -- so `for x (a b)` and `source` of a file with
   an unterminated brace both failed with status 2 and complete silence.
   `reported` is set by unexpected() when it has already spoken; everything
   else lands here and gets bash's wording, prefixed with the file and line
   when a file is being sourced (error_where.c). */
void	report_parse_error(t_shell *state, t_parser *parser,
				t_deque_tok *tt)
{
	char	*where;

	if (parser->res == RES_GETMOREINPUT)
	{
		where = parse_err_ctx(state, tt, tt->base + ft_strlen(tt->base));
		ft_eprintf("%s: syntax error: unexpected end of file\n", where);
		xfree(where);
		zsh_brace_hint(state, tt);
	}
	else if (!parser->reported)
		unexpected(state, parser, (t_ast_node){0}, tt);
	set_cmd_status(state, res_status(2));
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
   printed its own message during the parse.

   Stream mode, the same one the input driver uses: a statement ends at
   the newline that ends it, not at the end of the queue.  Without it
   parse_simple_list swallowed every `;`- and newline-joined command up to
   the error as ONE list, so `echo before` on line 1 never ran when line 2
   was broken -- bash runs it (issue #113), because bash reads a file one
   complete command at a time. */
int	run_one_stmt(t_shell *state, t_deque_tok *tt, bool *stop)
{
	t_parser	parser;
	t_ast_node	ast;
	int			status;

	parser = (t_parser){.res = RES_OK, .stream = 1};
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
