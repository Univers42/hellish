/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   input_private.h                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 15:21:06 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/24 01:25:10 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef INPUT_PRIVATE_H
# define INPUT_PRIVATE_H

# include "shell.h"
# include "sh_input.h"
# include <stdbool.h>
# include "lexer.h"
# include "parser.h"
# include "prompt.h"
# include "ast.h"

bool	ends_with_bs_nl(t_string s);
bool	is_empty_token_list(t_deque_tt *tokens);
int		readline_cmd(t_shell *state, char **prompt);
void	extend_bs(t_shell *state);
int		get_more_tokens(t_shell *state, char **prompt, t_deque_tt *tt);
bool	try_parse_tokens(t_shell *state, t_parser *parser,
			t_deque_tt *tt, char **prompt);
void	get_more_input_parser(t_shell *state,
			t_parser *parser, char **prompt, t_deque_tt *tt);
void	parse_and_execute_input(t_shell *state);

void	debug_parser_cleanup(t_shell *state, t_deque_tt *tt, char **prompt);
void	debug_parser_loop(t_shell *state, t_parser *parser,
			char **prompt, t_deque_tt *tt);
void	debug_lexer_print_tokens(t_shell *state, t_deque_tt *tt);
void	debug_lexer_cleanup(t_shell *state, t_deque_tt *tt, char **prompt);
void	reset_status_and_prompt(t_shell *state, char **prompt);
void	handle_ctrl_c(t_shell *state, t_deque_tt *tt, char **prompt);
int		handle_eof(int s, t_shell *state);
int		handle_interrupt(int s, t_shell *state, t_deque_tt *tt, char **prompt);
void	debug_parser_print_ast(t_shell *state,
			t_parser *parser, t_ast_node parsed);
void	print_and_cleanup_tokens(t_shell *state,
			t_deque_tt *tt,
			char **prompt);
#endif