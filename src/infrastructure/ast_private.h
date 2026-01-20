/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ast_private.h                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 15:20:09 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/20 17:00:51 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef AST_PRIVATE_H
# define AST_PRIVATE_H
# include "shell.h"
# include "ast.h"
# include <stdint.h>
# include <unistd.h>
# include <stdlib.h>
# include "parser.h"

typedef struct s_print_seq_ctx
{
	t_ast_node	*children;
	int			*depth_stack;
	int			depth;
}	t_print_seq_ctx;

char	*node_name(t_ast_t tn);
void	print_node(t_ast_node node);
void	print_token_str(t_ast_node node, int outfd);
void	print_dot_node(t_shell *state, t_ast_node node, uint32_t id, int outfd);
void	print_ast_dot(t_shell *state, t_ast_node node);
void	print_ast_tree(t_ast_node node);
void	ast_postorder_traversal(t_ast_node *node, void (*f)(t_ast_node *node));
void	free_node(t_ast_node *node);
void	free_ast(t_ast_node *node);
char	*get_node_color(t_ast_t tn);
char	*get_node_shape(t_ast_t tn);
void	print_tree_recursive(t_ast_node node, int *depth_stack, int depth);
void	print_tree_prefix(int *depth_stack, int depth, int is_last);
void	print_node_line(t_ast_node node);
void	print_op_line(t_ast_node op_node);
void	print_sequence_range_ctx(t_print_seq_ctx *ctx, int start, int end);
#endif