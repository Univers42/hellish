/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   parser.h                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/10 02:22:59 by marvin            #+#    #+#             */
/*   Updated: 2026/01/10 02:22:59 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

/* Recursive-descent parser public API.
   The parser turns a t_deque_tok into a t_ast_node tree.  The top-level
   entry is parse_tokens().  Each parse_* function corresponds to a POSIX
   grammar production; they consume tokens from the deque and return the
   corresponding AST subtree.  parse_stack is used for heredoc ordering. */
#ifndef PARSER_H
# define PARSER_H

# include "libft.h"
# include "shell.h"
# include "executor.h"
# include "ast.h"
# include "lexer.h"

/* Parser state passed through all parse_* calls.  res carries a
   parse-time error or RES_GETMOREINPUT (incomplete input, need more). */
typedef struct s_parser
{
	t_result_type	res; /* parse result / error code */
	t_vec			parse_stack; /* heredoc ordering stack */
}	t_parser;

/* Token classification helpers: is this token a redirect operator?
   a list separator (;, &&, ||)? a valid command word? a proc-sub opener? */
bool		is_redirect(t_tt tt);
bool		is_simple_list_op(t_tt tt);
bool		is_simple_cmd_token(t_tt tt);
bool		is_proc_sub(t_tt tt);

/* Grammar production functions (POSIX shell grammar, top-down). */
t_ast_node	parse_word(t_deque_tok *tokens);
t_ast_node	parse_redirect(t_shell *state,
				t_parser *parser, t_deque_tok *tokens);
t_ast_node	parse_simple_command(t_shell *state, t_parser *res,
				t_deque_tok *tokens);
t_ast_node	parse_command(t_shell *state, t_parser *parser,
				t_deque_tok *tokens);
bool		is_compund_list_op(t_tt tt);
bool		parse_compound_list_s(t_shell *state, t_parser *parser,
				t_deque_tok *tokens, t_ast_node *ret);
t_ast_node	parse_compound_list(t_shell *state,
				t_parser *parser, t_deque_tok *tokens);
int			parse_simple_list_s(t_shell *state, t_parser *parser,
				t_deque_tok *tokens, t_ast_node *ret);
t_ast_node	parse_simple_list(t_shell *state,
				t_parser *parser, t_deque_tok *tokens);
t_ast_node	create_subtoken_node(t_token t,
				int offset, int end_offset, t_tt tt);
t_ast_node	parse_subshell(t_shell *state,
				t_parser *parser, t_deque_tok *tokens);
t_ast_node	parse_pipeline(t_shell *state,
				t_parser *parser, t_deque_tok *tokens);
t_ast_node	parse_tokens(t_shell *state,
				t_parser *parser, t_deque_tok *tokens);

/* Process substitution parser */
t_ast_node	parse_proc_sub(t_shell *state, t_parser *parser,
				t_deque_tok *tokens);

#endif