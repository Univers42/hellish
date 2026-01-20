/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   parser_private.h                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 19:03:35 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/20 19:32:57 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef PARSER_PRIVATE_H
# define PARSER_PRIVATE_H

# include "shell.h"
# include <stdbool.h>
# include "parser.h"
# include "shell.h"
# include "lexer.h"
# include "parser.h"
# include "ast.h"
# include "decomposer.h"
# include "sys.h"

int			parse_simple_list_s(t_shell *state, t_parser *parser,
					t_deque_tt *tokens, t_ast_node *ret);
t_ast_node	parse_simple_list(t_shell *state,
					t_parser *parser, t_deque_tt *tokens);
t_ast_node	unexpected(t_shell *state, t_parser *parser, t_ast_node ret, t_deque_tt *tokens);
t_ast_node	create_subtoken_node(t_token t, int offset, int end_offset, t_tt tt);
t_ast_node	parse_subshell(t_shell *state, t_parser *parser, t_deque_tt *tokens);
t_ast_node	parse_pipeline(t_shell *state, t_parser *parser, t_deque_tt *tokens);
t_ast_node	parse_tokens(t_shell *state, t_parser *parser, t_deque_tt *tokens);
bool		is_compund_list_op(t_tt tt);
bool		parse_compound_list_s(t_shell *state, t_parser *parser,
				t_deque_tt *tokens, t_ast_node *ret);
t_ast_node	parse_compound_list(t_shell *state,
				t_parser *parser, t_deque_tt *tokens);
t_ast_node	unexpected(t_shell *state, t_parser *parser,
				t_ast_node ret, t_deque_tt *tokens);
bool		is_redirect(t_tt tt);
bool		is_proc_sub(t_tt tt);
bool		is_simple_cmd_token(t_tt tt);
t_ast_node	parse_word(t_deque_tt *tokens);
t_ast_node	parse_redirect(t_shell *state, t_parser *parser, t_deque_tt *tokens);
t_ast_node	parse_proc_sub(t_shell *state, t_parser *parser, t_deque_tt *tokens);
t_ast_node	parse_simple_command(t_shell *state, t_parser *res,
				t_deque_tt *tokens);
t_ast_node	parse_command(t_shell *state, t_parser *parser, t_deque_tt *tokens);
bool is_semicolon_or_newline_before_brace_right(t_ast_node *ret, t_deque_tt *tokens);

# endif