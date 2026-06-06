/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ast_utils.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 15:18:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/03/14 14:07:05 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ast_private.h"

char	*node_name_compound(t_ast_type tn);

/* Map a node type enum to its string name for debug output (DOT labels and the
   text tree). Simple types are handled here; compound types (if/while/for ...)
   are split into node_name_compound to keep each function under 25 lines. */
char	*node_name(t_ast_type tn)
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
	if (tn == AST_PROC_SUB)
		return ("AST_PROC_SUB");
	return (node_name_compound(tn));
}

/* Recursive S-expression dump: "(TYPE >token< (child ...) ...)". Useful as a
   compact one-liner representation during interactive debugging sessions. */
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

/* Write the token's text to outfd with minimal escaping so it is safe inside a
   DOT double-quoted label: backslash, double-quote, and single-quote are all
   escaped. Characters that do not need escaping are emitted verbatim. */
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

/* Emit one DOT node declaration and its edges to children, using a random
   uint32 id per child (from the shell's PRNG) so node names never clash across
   recursive calls. Edge labels show the child index for ordered constructs like
   for-loop headers. Colors and shapes are assigned by type (see get_node_*). */
void	print_dot_node(t_shell *state, t_ast_node node, uint32_t id, int outfd)
{
	size_t		i;
	uint32_t	r;

	ft_fdprintf(outfd, "	n%u [label=\"%s", id, node_name(node.node_type));
	if (node.node_type == AST_TOKEN)
	{
		ft_fdprintf(outfd, "\\n%s: >", tt_to_str(node.token.tt));
		print_token_str(node, outfd);
		ft_fdprintf(outfd, "<");
	}
	ft_fdprintf(outfd, "\", style=filled, fillcolor=\"%s\", shape=%s",
		get_node_color(node.node_type), get_node_shape(node.node_type));
	if (node.children.len > 0)
		ft_fdprintf(outfd, ", penwidth=2");
	ft_fdprintf(outfd, "];\n");
	i = 0;
	while (i < node.children.len)
	{
		r = random_uint32(&state->prng);
		ft_fdprintf(outfd, "	n%u -> n%u [label=\"%d\"];\n", id, r, (int)i);
		print_dot_node(state, *(t_ast_node *)vec_idx(&node.children, i),
			r, outfd);
		i++;
	}
}

/* Write out.dot and also dump the text tree to stdout. The DOT boilerplate sets
   up a top-to-bottom digraph with a light background; paste the file into
   https://dreampuf.github.io/GraphvizOnline/ to visualise without installing
   Graphviz locally. The text tree print follows immediately in the terminal for
   a quick structural overview. */
void	print_ast_dot(t_shell *state, t_ast_node node)
{
	int	outfd;

	outfd = open("out.dot", O_WRONLY | O_TRUNC | O_CREAT, 0666);
	if (outfd < 0)
	{
		warning_error_errno();
		return ;
	}
	ft_fdprintf(outfd, "digraph AST {\n");
	ft_fdprintf(outfd, "	rankdir=TB;\n");
	ft_fdprintf(outfd, "	node [fontname=\"Arial\", fontsize=12];\n");
	ft_fdprintf(outfd, "	edge [fontname=\"Arial\", fontsize=10];\n");
	ft_fdprintf(outfd, "	bgcolor=\"#F5F5F5\";\n\n");
	print_dot_node(state, node, 0, outfd);
	ft_fdprintf(outfd, "\n	labelloc=\"t\";\n");
	ft_fdprintf(outfd, "	label=\"AST Visualization\";\n");
	ft_fdprintf(outfd, "	fontsize=20;\n");
	ft_fdprintf(outfd, "}\n");
	close(outfd);
	ft_printf("\033[1;32m✓ DOT file written to 'out.dot'\033[0m\n");
	ft_printf("  Generate image with: dot -Tpng out.dot -o ast.png\n");
	print_ast_tree(node);
}
