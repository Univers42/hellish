/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   reparser.h                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/10 02:24:54 by marvin            #+#    #+#             */
/*   Updated: 2026/01/10 02:24:54 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

# ifndef REPARSER_H
# define REPARSER_H

# include "common.h"

void		reparse_dquote(t_ast_node *ret, int *i, t_token t);
void		reparse_assignment_words(t_ast_node *node);
void		reparse_envvar(t_ast_node *ret, int *i, t_token t, t_tt tt);
void		reparse_words(t_ast_node *node);
t_ast_node	reparse_word(t_token t);
//[a-zA-Z_]
bool		is_var_name_p1(char c);
//[a-zA-Z0-9_]
bool		is_var_name_p2(char c);
t_ast_node	create_subtoken_node(t_token t,
				int offset, int end_offset, t_tt tt);


#endif