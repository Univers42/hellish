/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   input_private.h                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 15:21:06 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/20 16:41:29 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef INPUT_PRIVATE_H
# define INPUT_PRIVATE_H

# include "shell.h"
# include "input.h"
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

#endif