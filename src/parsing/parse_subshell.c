/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   parse_subshell.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 19:22:21 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/27 16:18:40 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "parser_private.h"

/* Parse a subshell command: ( compound_list ). The leading `(` is TT_BRACE_LEFT
   (the lexer's operator token), not TT_LBRACE (the keyword). We push
   TT_BRACE_LEFT onto the parse stack so error messages can tell the user
   which open `(` is unclosed. The trailing `)` must be present; if not,
   unexpected() sets RES_ERR. */
t_ast_node	parse_subshell(t_shell *state,
						t_parser *parser,
						t_deque_tok *tokens)
{
	t_ast_node	ret;
	t_token		peek_t;

	ret = (t_ast_node){.node_type = AST_SUBSHELL};
	vec_init(&ret.children);
	ret.children.elem_size = sizeof(t_ast_node);
	vec_push_int(&parser->parse_stack, TT_BRACE_LEFT);
	peek_t = ltok2tok(*(t_ltoken *)deque_peek(&tokens->deqtok), tokens->base);
	if (peek_t.tt != TT_BRACE_LEFT)
		return (unexpected(state, parser, ret, tokens));
	deque_pop_start(&tokens->deqtok);
	push_parsed_compound_list(state, parser, tokens, &ret);
	if (parser->res != RES_OK)
		return (ret);
	peek_t = ltok2tok(*(t_ltoken *)deque_peek(&tokens->deqtok), tokens->base);
	if (peek_t.tt != TT_BRACE_RIGHT)
		return (unexpected(state, parser, ret, tokens));
	return (deque_pop_start(&tokens->deqtok),
		vec_pop(&parser->parse_stack), ret);
}

/* `} always { ... }` -- zsh's finally.  The second block runs whatever the
** first did, including after a `return` or a failing command, which is why
** plugins that touch the line editor use it: oh-my-zsh's sudo plugin has to
** put the buffer back no matter how its case statement exited.
**
** Stored as a SECOND CHILD of the brace group rather than a node type of its
** own, so every existing consumer -- the executor's dispatch, redirection,
** the cloner, free_ast -- keeps working untouched and only the one place
** that runs the group has to know.  children.len == 2 is the whole signal.
**
** What gets stored is the cleanup block's COMPOUND LIST, not the brace group
** wrapping it, because execute_tree_node has no case for AST_BRACE_GROUP --
** that type is reached through run_compound.  Storing the wrapper made the
** cleanup silently do nothing: the dispatch fell through to an ft_assert
** that a release build compiles out, so the block was parsed, held, and
** never run, with no error anywhere.
**
** Gated on the dialect: `always` is an ordinary word in bash, and a brace
** group followed by a command called `always` has to stay exactly that. */
static void	try_always_block(t_shell *state, t_parser *parser,
				t_deque_tok *tokens, t_ast_node *ret)
{
	t_ltoken	*peek;
	t_ast_node	group;
	t_ast_node	inner;

	if (!zsh_mode(state))
		return ;
	peek = (t_ltoken *)deque_peek(&tokens->deqtok);
	if (peek->tt != TT_WORD || peek->len != 6
		|| !kw_eq(tokens->base + peek->off, "always", 6))
		return ;
	(void)deque_pop_start(&tokens->deqtok);
	skip_newlines(tokens);
	if ((*(t_ltoken *)deque_peek(&tokens->deqtok)).tt != TT_LBRACE)
		return ((void)unexpected(state, parser, *ret, tokens));
	group = parse_brace_group(state, parser, tokens);
	if (parser->res != RES_OK || !group.children.len)
		return (free_ast(&group));
	inner = *(t_ast_node *)vec_idx(&group.children, 0);
	xfree(group.children.ctx);
	ast_push_child(ret, &inner);
}

/* Parse a brace group: { compound_list ; } -- runs in the current shell, no
   fork. The braces are keyword tokens (TT_LBRACE / TT_RBRACE), distinct from
   the operator tokens that open/close subshells (TT_BRACE_LEFT/RIGHT). The
   trailing `}` must be preceded by `;` or newline (the grammar enforces this
   via is_separator_before_terminator in compound_list.c). */
t_ast_node	parse_brace_group(t_shell *state,
						t_parser *parser,
						t_deque_tok *tokens)
{
	t_ast_node	ret;
	t_token		peek_t;

	ret = (t_ast_node){.node_type = AST_BRACE_GROUP};
	vec_init(&ret.children);
	ret.children.elem_size = sizeof(t_ast_node);
	vec_push_int(&parser->parse_stack, TT_LBRACE);
	peek_t = ltok2tok(*(t_ltoken *)deque_peek(&tokens->deqtok), tokens->base);
	if (peek_t.tt != TT_LBRACE)
		return (unexpected(state, parser, ret, tokens));
	deque_pop_start(&tokens->deqtok);
	push_parsed_compound_list(state, parser, tokens, &ret);
	if (parser->res != RES_OK)
		return (ret);
	peek_t = ltok2tok(*(t_ltoken *)deque_peek(&tokens->deqtok), tokens->base);
	if (peek_t.tt != TT_RBRACE)
		return (unexpected(state, parser, ret, tokens));
	deque_pop_start(&tokens->deqtok);
	vec_pop(&parser->parse_stack);
	try_always_block(state, parser, tokens, &ret);
	return (ret);
}
