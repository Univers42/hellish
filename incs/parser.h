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

#ifndef PARSER_H
# define PARSER_H

# include "libft.h"
# include "shell.h"
# include "executor.h"
# include "ast.h"
# include "lexer.h"

typedef struct s_parser
{
	t_result_type	res;
	t_vec			parse_stack;
}	t_parser;

bool		is_redirect(t_tt tt);
bool		is_simple_list_op(t_tt tt);
bool		is_simple_cmd_token(t_tt tt);
bool		is_proc_sub(t_tt tt);

// process of parsing
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