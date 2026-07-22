/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   parse_pipeline.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 19:22:03 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/27 16:20:47 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "parser_private.h"

/* Consume `| command` pairs until no pipe remains. After each `|` we skip
   any newlines (POSIX allows line continuation inside a pipeline) and check
   for TT_END -- a trailing `|` at end-of-input means the user wants to type
   more, so we signal RES_GETMOREINPUT instead of an error. Returns 0 when
   the loop finishes cleanly, 1 on incomplete input, 2 on parse error. */
static int	process_pipeline_pipes(t_shell *state, t_parser *parser,
								t_deque_tok *tokens, t_ast_node *ret)
{
	t_ast_node	tmp_node;

	while ((*(t_ltoken *)deque_peek(&tokens->deqtok)).tt == TT_PIPE)
	{
		(void)deque_pop_start(&tokens->deqtok);
		while ((*(t_ltoken *)deque_peek(&tokens->deqtok)).tt == TT_NEWLINE)
			(void)deque_pop_start(&tokens->deqtok);
		if ((*(t_ltoken *)deque_peek(&tokens->deqtok)).tt == TT_END)
		{
			parser->res = RES_GETMOREINPUT;
			return (1);
		}
		tmp_node = parse_command(state, parser, tokens);
		ast_push_child(ret, &tmp_node);
		if (parser->res != RES_OK)
			return (2);
	}
	return (0);
}

/* Parse a pipeline: [!] command [| command]...
   The leading `!` negates the pipeline's exit status; multiple `!` toggle
   the flag, so `!! cmd` has exit status == cmd's. A bare `!` at the end of
   input is a valid zero-command pipeline in bash (status 1, the negation
   of an empty pipeline's 0) — the executor already computes that, so we
   just return the childless node instead of demanding a command. The
   parse_stack entry for TT_PIPE lets the error reporter tell the user
   which construct is open. AST: AST_COMMAND_PIPELINE with ret.negate and
   one AST_COMMAND child per stage. A single-command pipeline with no `!`
   is still a pipeline node; the executor normalises it. */
static bool	eat_bangs(t_deque_tok *tokens, t_ast_node *ret)
{
	bool	saw;

	saw = false;
	while ((*(t_ltoken *)deque_peek(&tokens->deqtok)).tt == TT_BANG)
	{
		(void)deque_pop_start(&tokens->deqtok);
		ret->negate = !ret->negate;
		saw = true;
	}
	return (saw);
}

t_ast_node	parse_pipeline(t_shell *state,
				t_parser *parser,
				t_deque_tok *tokens)
{
	t_ast_node	ret;
	int			r;

	ret = create_node_type(AST_COMMAND_PIPELINE);
	vec_init(&ret.children);
	ret.children.elem_size = sizeof(t_ast_node);
	if (eat_bangs(tokens, &ret)
		&& ((*(t_ltoken *)deque_peek(&tokens->deqtok)).tt == TT_END
			|| (*(t_ltoken *)deque_peek(&tokens->deqtok)).tt == TT_NEWLINE))
		return (ret);
	push_cmd_parsed(state, parser, tokens, &ret);
	if (parser->res != RES_OK)
		return (ret);
	vec_push_int(&parser->parse_stack, TT_PIPE);
	r = process_pipeline_pipes(state, parser, tokens, &ret);
	vec_pop(&parser->parse_stack);
	if (r == 1 || r == 2)
		return (ret);
	return (ret);
}
