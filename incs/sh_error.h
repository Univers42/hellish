/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   sh_error.h                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/11 20:16:21 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/27 16:13:17 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SH_ERROR_H
# define SH_ERROR_H

# include "ast.h"
# include "lexer.h"
# include "parser.h"

void				err_cmd_not_found(t_shell *state, char *cmd);
void				err_no_path_var(t_shell *state, char *cmd);
void				err_1_errno(t_shell *state, char *p1);
void				err_2(t_shell *state, char *p1, char *p2);
t_ast_node			unexpected(t_shell *state, t_parser *parser,
				t_ast_node ret, t_deque_tok *tokens);

#endif