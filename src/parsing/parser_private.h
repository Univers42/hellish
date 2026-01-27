/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   parser_private.h                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 19:03:35 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/27 16:07:19 by dlesieur         ###   ########.fr       */
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
				t_deque_tok *tokens, t_ast_node *ret);
t_ast_node	parse_simple_list(t_shell *state,
				t_parser *parser, t_deque_tok *tokens);
t_ast_node	unexpected(t_shell *state, t_parser *parser,
				t_ast_node ret,
				t_deque_tok *tokens);
t_ast_node	create_subtoken_node(t_token t,
				int offset,
				int end_offset,
				t_tt tt);
t_ast_node	parse_subshell(t_shell *state,
				t_parser *parser,
				t_deque_tok *tokens);
t_ast_node	parse_pipeline(t_shell *state,
				t_parser *parser,
				t_deque_tok *tokens);
t_ast_node	parse_tokens(t_shell *state,
				t_parser *parser,
				t_deque_tok *tokens);
bool		is_compound_list_op(t_tt tt);
bool		parse_compound_list_s(t_shell *state, t_parser *parser,
				t_deque_tok *tokens, t_ast_node *ret);
t_ast_node	parse_compound_list(t_shell *state,
				t_parser *parser, t_deque_tok *tokens);
t_ast_node	unexpected(t_shell *state, t_parser *parser,
				t_ast_node ret, t_deque_tok *tokens);
bool		is_redirect(t_tt tt);
bool		is_proc_sub(t_tt tt);
bool		is_simple_cmd_token(t_tt tt);
t_ast_node	parse_word(t_deque_tok *tokens);
t_ast_node	parse_redirect(t_shell *state,
				t_parser *parser,
				t_deque_tok *tokens);
t_ast_node	parse_proc_sub(t_shell *state,
				t_parser *parser,
				t_deque_tok *tokens);
t_ast_node	parse_simple_command(t_shell *state, t_parser *res,
				t_deque_tok *tokens);
t_ast_node	parse_command(t_shell *state,
				t_parser *parser,
				t_deque_tok *tokens);
bool		is_semicolon_or_newline_before_brace_right(t_ast_node *ret,
				t_deque_tok *tokens);
void		handle_arith_error_print(t_shell *state,
				bool has_inner_paren,
				t_token last_word);
int			handle_arith_error(t_shell *state,
				t_parser *parser,
				t_deque_tok *tokens,
				t_ast_node *ret);
void		handle_arith_error_collect(t_deque_tok *tokens,
				t_string *expr_buf,
				bool *has_inner_paren,
				t_token *last_word);
bool		is_end_token(t_deque_tok *tokens);
bool		is_newline_token(t_deque_tok *tokens);
void		handle_unexpected_token(t_shell *state,
				t_parser *parser,
				t_ast_node ret,
				t_deque_tok *tokens);
void		init_ast_node_children(t_ast_node *node, t_ast_type type);
void		push_token_child(t_ast_node *parent, t_token tok);
void		push_cmd_parsed(t_shell *state,
				t_parser *parser,
				t_deque_tok *tokens,
				t_ast_node *ret);
void		push_parsed_pipeline_child(t_shell *state,
				t_parser *parser,
				t_deque_tok *tokens,
				t_ast_node *ret);
void		push_parsed_proc_sub(t_shell *state,
				t_parser *parser,
				t_deque_tok *tokens,
				t_ast_node *ret);
void		push_parsed_redirect(t_shell *state,
				t_parser *parser,
				t_deque_tok *tokens,
				t_ast_node *ret);
void		push_parsed_word(t_deque_tok *tokens, t_ast_node *ret);
bool		check_res_ok(t_parser *res);
void		push_parsed_compound_list(t_shell *state, t_parser *parser,
				t_deque_tok *tokens, t_ast_node *ret);
void		handle_arith_error_print(t_shell *state,
				bool has_inner_paren,
				t_token last_word);
int			push_simple_list_op_token(t_parser *parser,
				t_deque_tok *tokens,
				t_ast_node *ret,
				t_tt *out_next);
int			check_newlines_and_end(t_parser *parser,
				t_deque_tok *tokens,
				t_tt next);
void		add_op_token_child(t_ast_node *ret, t_token op_tok);
int			proc_sub_handle_eof(t_parser *parser,
				t_deque_tok *tokens,
				t_string *cmd_str,
				t_token curr);

#endif