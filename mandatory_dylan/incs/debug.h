/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   debug.h                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/07 15:37:49 by marvin            #+#    #+#             */
/*   Updated: 2026/01/07 15:37:49 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef DEBUG_H
# define DEBUG_H

# include "common.h"

/* Lexer debug helpers */
# ifdef DEBUG_LEXER
char	*tt_to_str(t_tt tt);
void	print_tokens(t_deque *tokens);
# endif

/* AST debug helpers */
# ifdef DEBUG_AST
char	*node_name(t_ast_type tn);
void	print_node(t_ast_node *node);
void	print_ast_dot(t_prng_state *prng, t_ast_node *node);
# endif

#endif /* DEBUG_H */