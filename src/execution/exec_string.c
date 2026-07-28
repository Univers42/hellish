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

int		run_parsed(t_shell *state, t_ast_node *ast);
bool	must_stop(t_shell *state);

/* Lex `str` once, then parse + execute it one statement at a time (the REPL
   normally feeds the parser one line at a time). Used by eval, command and the
   dot/source builtin. `str` must stay valid for the whole call. */
/* Drop any leading newline/semicolon tokens from the token queue before
   the next statement is parsed.  exec_string feeds multiple statements
   from a single token stream, so after each statement the queue may have
   stale statement-separators that would confuse the next parse call. */
static void	skip_delimiters(t_deque_tok *tt)
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
   return outside function, exit, SIGTERM unwind). */
static int	run_one_stmt(t_shell *state, t_deque_tok *tt, bool *stop)
{
	t_parser	parser;
	t_ast_node	ast;
	int			status;

	parser = (t_parser){.res = RES_OK};
	vec_init(&parser.parse_stack);
	parser.parse_stack.elem_size = sizeof(int);
	ast = parse_simple_list(state, &parser, tt);
	if (parser.res == RES_OK)
		status = run_parsed(state, &ast);
	else
	{
		set_cmd_status(state, res_status(2));
		status = 2;
	}
	free_ast(&ast);
	xfree(parser.parse_stack.ctx);
	*stop = (parser.res != RES_OK || must_stop(state));
	if (!*stop)
		skip_delimiters(tt);
	return (status);
}

/* Lex `str` once into a token queue, then parse + execute it statement
   by statement until end-of-tokens or a stop condition.  Clearing
   func_return at the end prevents a `return` inside eval from propagating
   out of exec_string into the caller's function frame. */
static int	exec_string_inner(t_shell *state, char *str)
{
	t_deque_tok	tt;
	int			status;
	bool		stop;

	tt = (t_deque_tok){0};
	deque_init(&tt.deqtok, 100, sizeof(t_ltoken));
	tokenizer(str, &tt);
	reclassify_keywords(&tt);
	status = 0;
	stop = false;
	skip_delimiters(&tt);
	while (!stop && ((t_ltoken *)deque_peek(&tt.deqtok))->tt != TT_END)
		status = run_one_stmt(state, &tt, &stop);
	state->func_return = 0;
	xfree(tt.deqtok.buff);
	return (status);
}

/* Extract heredoc bodies up front (so they aren't parsed as commands),
   feed them to the heredoc reader via state->hd_src, and run the stripped
   text.  A string without << (or where splitting finds nothing) runs
   as-is, under whatever hd_src the caller already installed. */
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
		status = exec_string_inner(state, stripped);
		xfree(stripped);
	}
	else
		status = exec_string_inner(state, str);
	return (status);
}

/* Alias-expand the line, then execute it, routing any heredoc bodies
   aside first.  hd_src/hd_pos are saved/restored so nested command
   substitutions each get their own body stream. */
int	exec_string(t_shell *state, char *str)
{
	char	*bodies;
	char	*prev_src;
	size_t	prev_pos;
	int		status;

	prev_src = state->hd_src;
	prev_pos = state->hd_pos;
	bodies = NULL;
	str = alias_scan_line(&state->aliases, str);
	status = exec_split_heredocs(state, str, &bodies);
	xfree(str);
	xfree(bodies);
	state->hd_src = prev_src;
	state->hd_pos = prev_pos;
	return (status);
}
