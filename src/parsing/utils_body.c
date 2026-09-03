/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   utils_body.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/03 12:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/09/03 12:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "parser_private.h"

/* One pipeline as a body of its own.
**
** A function body that is not a brace group (`f() ( ... )`, `f() while
** ...; done`) and zsh's short loop body (`for x (a b) cmd`) are each ONE
** command in the grammar, but the executor runs bodies as lists: a
** compound list of pipelines is what `{ ... }` and `do ... done` hand it,
** and execute_tree_node has no case for a bare command -- a command is
** only ever a pipeline's child, and running one directly tripped the
** exhausted-node-types assert.  So the one command is parsed as the one
** pipeline of a one-element compound list, and the body executes through
** exactly the path every other body takes.  parse_pipeline rather than
** parse_command so `! cmd` is accepted where the shells accept it. */
t_ast_node	parse_body_pipeline(t_shell *state, t_parser *parser,
				t_deque_tok *tokens)
{
	t_ast_node	body;

	init_ast_node_children(&body, AST_COMPOUND_LIST);
	push_parsed_pipeline_child(state, parser, tokens, &body);
	return (body);
}
