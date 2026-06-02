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

static int	run_parsed(t_shell *state, t_ast_node *ast)
{
	t_executable_node	exe;
	t_execution_state	res;

	reparse_words(ast);
	reparse_assignment_words(ast);
	exe = create_exe_node(0, 1, ast, true);
	vec_init(&exe.redirs);
	exe.redirs.elem_size = sizeof(int);
	gather_heredocs(state, ast, false);
	res = execute_tree_node(state, &exe);
	set_cmd_status(state, res);
	return (res.status);
}

static bool	must_stop(t_shell *state)
{
	return (state->should_exit || state->func_return || state->loop_break
		|| state->loop_continue || get_g_sig()->should_unwind);
}

/* Lex `str` once, then parse + execute it one statement at a time (the REPL
   normally feeds the parser one line at a time). Used by eval, command and the
   dot/source builtin. `str` must stay valid for the whole call. */
static int	exec_string_inner(t_shell *state, char *str)
{
	t_deque_tok	tt;
	t_parser	parser;
	t_ast_node	ast;
	int			status;

	tt = (t_deque_tok){0};
	deque_init(&tt.deqtok, 100, sizeof(t_token));
	tokenizer(str, &tt);
	reclassify_keywords(&tt);
	status = 0;
	while (((t_token *)deque_peek(&tt.deqtok))->tt != TT_END)
	{
		parser = (t_parser){.res = RES_OK};
		vec_init(&parser.parse_stack);
		parser.parse_stack.elem_size = sizeof(int);
		ast = parse_simple_list(state, &parser, &tt);
		if (parser.res == RES_OK)
			status = run_parsed(state, &ast);
		else
			status = (set_cmd_status(state, res_status(2)), 2);
		free_ast(&ast);
		free(parser.parse_stack.ctx);
		if (parser.res != RES_OK || must_stop(state))
			break ;
		while (((t_token *)deque_peek(&tt.deqtok))->tt == TT_NEWLINE
			|| ((t_token *)deque_peek(&tt.deqtok))->tt == TT_SEMICOLON)
			(void)deque_pop_start(&tt.deqtok);
	}
	state->func_return = 0;
	return (free(tt.deqtok.buff), status);
}

/* Extract heredoc bodies up front (so they aren't parsed as commands) and feed
   them to the heredoc reader via state->hd_src. hd_src is saved/restored so
   nested command substitutions each get their own body stream. */
int	exec_string(t_shell *state, char *str)
{
	char	*stripped;
	char	*bodies;
	char	*prev_src;
	size_t	prev_pos;
	int		status;

	prev_src = state->hd_src;
	prev_pos = state->hd_pos;
	stripped = NULL;
	bodies = NULL;
	if (split_heredocs(str, &stripped, &bodies))
	{
		state->hd_src = bodies;
		state->hd_pos = 0;
		status = exec_string_inner(state, stripped);
		free(stripped);
	}
	else
		status = exec_string_inner(state, str);
	free(bodies);
	state->hd_src = prev_src;
	state->hd_pos = prev_pos;
	return (status);
}
