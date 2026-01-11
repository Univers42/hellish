/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   tree_utils.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 23:34:55 by marvin            #+#    #+#             */
/*   Updated: 2026/01/09 23:34:55 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "shell.h"
#include "ast.h"
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>

void	ast_postorder_traversal(t_ast_node *node, void (*f)(t_ast_node *node))
{
	size_t	i;

	i = 0;
	while (i < node->children.len)
	{
		ast_postorder_traversal(vec_idx(&node->children, i), f);
		i++;
	}
	f(node);
}

void	free_node(t_ast_node *node)
{
	if (node->node_type == AST_TOKEN && node->token.allocated)
		free(node->token.start);
	free(node->children.ctx);
	*node = (t_ast_node){};
}

void	free_ast(t_ast_node *node)
{
	ast_postorder_traversal(node, free_node);
}

char	*node_name(t_ast_t tn)
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

void	print_node(t_ast_node node)
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

void	print_token_str(t_ast_node node, int outfd)
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

void	print_dot_node(t_shell *state, t_ast_node node, uint32_t id, int outfd)
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

void	print_ast_dot(t_shell *state, t_ast_node node)
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
