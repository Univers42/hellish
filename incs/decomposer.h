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

#ifndef DECOMPOSER_H
# define DECOMPOSER_H

# include "ast.h"
# include "lexer.h"
# include <stdbool.h>

void		reparse_dquote(t_ast_node *ret, int *i, t_token t);
void		reparse_assignment_words(t_ast_node *node);
void		reparse_envvar(t_ast_node *ret, int *i, t_token t, t_tt tt);
void		reparse_words(t_ast_node *node);
t_ast_node	reparse_word(t_token t);

#endif
