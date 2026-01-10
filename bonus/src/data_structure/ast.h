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

# include "common.h"
# include "alias.h"
# include "sh"
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
void	free_ast(t_ast_node *node)

static inline char	*node_name(t_ast_t tn)
{
	if (tn == AST_COMMAND_PIPELINE)
		return ("AST_COMMAND_PIPELINE");
	if (tn == AST_REDIRECT)
		return ("AST_REDIRECT");
	if (tn == AST_WORD)
		return ("AST_WORD");
	if (tn == AST_TOKEN)
		return ("AST_TOKEN");
	if (tn == AST_SIMPLE_LIST)
		return ("AST_SIMPLE_LIST");
	if (tn == AST_SIMPLE_COMMAND)
		return ("AST_SIMPLE_COMMAND");
	if (tn == AST_SUBSHELL)
		return ("AST_SUBSHELL");
	if (tn == AST_COMPOUND_LIST)
		return ("AST_COMPOUND_LIST");
	if (tn == AST_COMMAND)
		return ("AST_COMMAND");
	if (tn == AST_ASSIGNMENT_WORD)
		return ("AST_ASSIGNMENT_WORD");
	ft_assert("Unreachable" == 0);
	return (0);
}

static inline void	print_node(t_ast_node node)
{
	size_t	i;

	ft_printf(" (%s", node_name(node.node_type));
	if (node.node_type == AST_TOKEN)
		ft_printf(" >%.*s<", node.token.len, node.token.start);
	i = 0;
	while (i < node.children.len)
		print_node(*(t_ast_node *)vec_idx(&node.children, i++));
	ft_printf(")");
}

static inline void	print_token_str(t_ast_node node, int outfd)
{
	int		i;
	char	c;

	i = 0;
	while (i < node.token.len)
	{
		c = node.token.start[i];
		if (c == '\\')
			ft_fdprintf(outfd, "\\\\");
		else if (c == '"')
			ft_fdprintf(outfd, "\\\"");
		else if (c == '\'')
			ft_fdprintf(outfd, "\\'");
		else
			ft_fdprintf(outfd, "%c", c);
		i++;
	}
}

static inline void	print_dot_node(t_state *state, t_ast_node node, uint32_t id, int outfd)
{
	size_t		i;
	uint32_t	r;

	ft_fdprintf(outfd, "	n%u [label=\"%s", id, node_name(node.node_type));
	if (node.node_type == AST_TOKEN)
	{
		ft_fdprintf(outfd, " %s: >", tt_to_str(node.token.tt));
		print_token_str(node, outfd);
		ft_fdprintf(outfd, "<");
	}
	ft_fdprintf(outfd, "\"];\n");
	i = 0;
	while (i < node.children.len)
	{
		r = random_uint32(&state->prng);
		ft_fdprintf(outfd, "	n%u -> n%u;\n", id, r);
		print_dot_node(state, *(t_ast_node *)vec_idx(&node.children, i), r, outfd);
		i++;
	}
}

static inline void	print_ast_dot(t_state *state, t_ast_node node)
{
	int	outfd;

	outfd = open("out", O_WRONLY | O_TRUNC | O_CREAT, 0666);
	if (outfd < 0)
		warning_error_errno();
	ft_fdprintf(outfd, "digraph G {\n");
	print_dot_node(state, node, 0, outfd);
	ft_fdprintf(outfd, "}\n");
	close(outfd);
}

void		print_node(t_ast_node node);
void		print_ast_dot(t_state *state, t_ast_node node);
void		ast_postorder_traversal(t_ast_node *node,
				void (*f)(t_ast_node *node));
void		free_ast(t_ast_node *node);

char		*tt_to_str(t_tt tt);
void		free_ast(t_ast_node *node);

void		print_tokens(t_deque_tt tokens);

#endif