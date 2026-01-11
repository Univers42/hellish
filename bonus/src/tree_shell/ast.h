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

# include "../../incs/shell.h"
# include "libft.h"
# include "alias.h"
# include "../lexer/lexer.h"
# include <fcntl.h>
# include "../infrastructure/error.h"

// Forward declarations to avoid circular dependency
struct s_vec_int;
typedef struct s_ast_node t_ast_node;

// Forward declare ft_fdprintf if header is missing
int ft_fdprintf(int fd, const char *fmt, ...);

typedef enum e_ast_t
{
	AST_SIMPLE_LIST,
	AST_COMMAND_PIPELINE,
	AST_REDIRECT,
	AST_SIMPLE_COMMAND,
	AST_SUBSHELL,
	AST_COMPOUND_LIST,
	AST_COMMAND,
	AST_WORD,
	AST_ASSIGNMENT_WORD,
	AST_TOKEN,
}	t_ast_t;

typedef struct s_ast_node
{
	t_ast_t		node_type;
	t_token		token;
	bool		has_redirect;
	int			redir_idx;
	t_vec_nd	children;
}	t_ast_node;

void	ast_postorder_traversal(t_ast_node *node, void (*f)(t_ast_node *node));
void	free_node(t_ast_node *node);
void	free_ast(t_ast_node *node);

char	*node_name(t_ast_t tn);
void	print_node(t_ast_node node);
void	print_token_str(t_ast_node node, int outfd);
void	print_dot_node(t_shell *state, t_ast_node node, uint32_t id, int outfd);
void	print_ast_dot(t_shell *state, t_ast_node node);

# endif