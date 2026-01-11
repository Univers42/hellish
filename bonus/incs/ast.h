/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ast.h                                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/10 01:48:00 by marvin            #+#    #+#             */
/*   Updated: 2026/01/10 01:48:00 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

# ifndef AST_H
# define AST_H

# include <stdint.h>
# include "public/token.h"

/* forward-declare shell to avoid circular includes */
typedef struct s_shell t_shell;

typedef enum e_ast_t
{
	AST_COMMAND_PIPELINE,
	AST_REDIRECT,
	AST_WORD,
	AST_TOKEN,
	AST_SIMPLE_LIST,
	AST_SIMPLE_COMMAND,
	AST_SUBSHELL,
	AST_COMPOUND_LIST,
	AST_COMMAND,
	AST_ASSIGNMENT_WORD,
} t_ast_t;

typedef struct s_ast_node
{
	t_ast_t	node_type;
	char		padding[4];
	t_vec	children; /* vector of t_ast_node */
	t_token	token; /* only valid when node_type == AST_TOKEN */
	bool	has_redirect;
	int		redir_idx;
} t_ast_node;

void	ast_postorder_traversal(t_ast_node *node, void (*f)(t_ast_node *node));
void	free_ast(t_ast_node *node);
void	print_ast_dot(t_shell *state, t_ast_node node);
void	print_dot_node(t_shell *state, t_ast_node node, uint32_t id, int outfd);

# endif