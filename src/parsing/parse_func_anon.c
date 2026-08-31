/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   parse_func_anon.c                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/30 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/30 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "parser_private.h"
#include "ft_glob.h"

/* zsh's ANONYMOUS FUNCTION: `() { ... }` defines a function with no name and
** runs it once, immediately.
**
** It is how a zsh plugin gets a private scope at load time, and both of the
** plugins ZLE was blamed for use it for exactly that:
**
**     () {
**       typeset -ga _ZSH_AUTOSUGGEST_BUILTIN_ACTIONS      # autosuggestions
**       ...
**     }
**     () {
**       (( REGION_ACTIVE )) || return                     # syntax-highlighting
**       integer min max
**       ...
**     }
**
** The `return` in the second one is why this cannot be a brace group. A brace
** group runs in the current shell, so `return` would return from whatever is
** SOURCING the plugin -- the rest of the file would silently not load. It has
** to be a real call frame, which is also what makes `local`, `integer` and
** `typeset` mean what the plugin expects.
**
** ARGUMENTS are not accepted. zsh allows `() { ... } a b`, no plugin in the
** corpus writes it, and the words would land after the body where the
** pipeline parser will report them -- loudly, rather than being dropped.
*/

/* `(` immediately followed by `)`, in the zsh dialect. Asked before the
   subshell branch, so it has to be the narrower test of the two: a subshell
   always has something between its parens (an empty one is a syntax error in
   every shell), and bash has no anonymous function at all, so neither answer
   changes outside zsh. */
bool	is_anon_func(t_deque_tok *tokens)
{
	t_ltoken	*t0;
	t_ltoken	*t1;

	if (!glob_zsh() || tokens->deqtok.len < 2)
		return (false);
	t0 = (t_ltoken *)deque_idx(&tokens->deqtok, 0);
	t1 = (t_ltoken *)deque_idx(&tokens->deqtok, 1);
	return (t0->tt == TT_BRACE_LEFT && t1->tt == TT_BRACE_RIGHT);
}

/* Pop the `()` and parse the body with the SAME parser the named form uses,
   so the two cannot disagree about what a body is. The node carries the body
   at children[0], which is the shape execute_anon_func expects and the shape
   AST_FUNCTION_DEF already had. */
t_ast_node	parse_anon_func(t_shell *state, t_parser *parser,
				t_deque_tok *tokens)
{
	t_ast_node	ret;
	t_ast_node	body;

	(void)deque_pop_start(&tokens->deqtok);
	(void)deque_pop_start(&tokens->deqtok);
	init_ast_node_children(&ret, AST_ANON_FUNC);
	skip_newlines(tokens);
	if ((*(t_ltoken *)deque_peek(&tokens->deqtok)).tt == TT_END)
		return (parser->res = RES_GETMOREINPUT, ret);
	body = parse_func_body(state, parser, tokens);
	ast_push_child(&ret, &body);
	return (ret);
}
