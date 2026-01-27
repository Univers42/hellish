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

#ifndef AST_H
# define AST_H

# include "libft.h"
# include "public/token.h"

/* Forward declaration */
typedef struct s_shell	t_shell;

typedef enum e_ast_type
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
	AST_PROC_SUB
}	t_ast_type;

typedef struct s_ast_node
{
	t_ast_type		node_type;
	t_token			token;
	t_vec			children;
	bool			has_redirect;
	int				redir_idx;
}	t_ast_node;

/* Vector type alias for AST nodes */
typedef t_vec			t_vec_nd;

/* Function prototypes */
void	free_ast(t_ast_node *node);
void	ast_postorder_traversal(t_ast_node *node, void (*f)(t_ast_node *node));
void	print_ast_dot(t_shell *state, t_ast_node node);
char	*node_name(t_ast_type tn);
void	print_node(t_ast_node node);

static inline t_ast_node	create_node_tok(t_ast_type type, t_token token)
{
	return ((t_ast_node)
		{
			.node_type = type,
			.token = token,
			.children = (t_vec){0},
		.has_redirect = false,
		.redir_idx = -1
	});
}

static inline t_ast_node	create_node_type(t_ast_type type)
{
	return ((t_ast_node)
		{
			.node_type = type,
			.token = (t_token){0},
		.children = (t_vec){0},
			.has_redirect = false,
			.redir_idx = -1
		});
}

#endif