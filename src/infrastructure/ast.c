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
#include "parser.h"

static void print_tree_recursive(t_ast_node node, int *depth_stack, int depth);

void ast_postorder_traversal(t_ast_node *node, void (*f)(t_ast_node *node))
{
	size_t i;

	i = 0;
	while (i < node->children.len)
	{
		ast_postorder_traversal(vec_idx(&node->children, i), f);
		i++;
	}
	f(node);
}

void free_node(t_ast_node *node)
{
	if (node->node_type == AST_TOKEN && node->token.allocated)
		free(node->token.start);
	free(node->children.ctx);
	*node = (t_ast_node){};
}

void free_ast(t_ast_node *node)
{
	ast_postorder_traversal(node, free_node);
}

/* Helper: Get color for node type in DOT format */
static char *get_node_color(t_ast_t tn)
{
	if (tn == AST_COMMAND_PIPELINE)
		return ("#FFB6C1");
	if (tn == AST_REDIRECT)
		return ("#87CEEB");
	if (tn == AST_WORD || tn == AST_TOKEN)
		return ("#90EE90");
	if (tn == AST_SIMPLE_LIST || tn == AST_COMPOUND_LIST)
		return ("#DDA0DD");
	if (tn == AST_SIMPLE_COMMAND || tn == AST_COMMAND)
		return ("#F0E68C");
	if (tn == AST_SUBSHELL)
		return ("#FFB347");
	if (tn == AST_ASSIGNMENT_WORD)
		return ("#B0E0E6");
	if (tn == AST_PROC_SUB)
		return ("#98FB98");
	return ("#FFFFFF");
}

/* Helper: Get shape for node type in DOT format */
static char *get_node_shape(t_ast_t tn)
{
	if (tn == AST_TOKEN || tn == AST_WORD)
		return ("ellipse");
	if (tn == AST_COMMAND || tn == AST_SIMPLE_COMMAND)
		return ("box");
	if (tn == AST_SUBSHELL)
		return ("octagon");
	if (tn == AST_REDIRECT)
		return ("diamond");
	return ("box");
}

/* Helper: Print tree branch characters */
static void print_tree_prefix(int *depth_stack, int depth, int is_last)
{
	int i;

	i = 0;
	while (i < depth - 1)
	{
		if (depth_stack[i])
			ft_printf("    ");
		else
			ft_printf("│   ");
		i++;
	}
	if (depth > 0)
	{
		if (is_last)
			ft_printf("└── ");
		else
			ft_printf("├── ");
	}
}

/* Helper: Print a single node header line (used by tree printer) */
static void print_node_line(t_ast_node node)
{
	ft_printf("\033[1;36m%s\033[0m", node_name(node.node_type));
	if (node.node_type == AST_TOKEN)
	{
		ft_printf(" \033[33m%s\033[0m: '\033[32m%.*s\033[0m'",
				  tt_to_str(node.token.tt), node.token.len, node.token.start);
	}
	ft_printf(" \033[90m[%d children]\033[0m\n", (int)node.children.len);
}

/* Helper: Print binary operator intro line (used for sequence tree) */
static void print_op_line(t_ast_node op_node)
{
	/* reuse token printing style for operator tokens */
	ft_printf("\033[1;33mOP %s\033[0m", tt_to_str(op_node.token.tt));
	if (op_node.node_type == AST_TOKEN)
		ft_printf(" \033[90m'%.*s'\033[0m", op_node.token.len, op_node.token.start);
	ft_printf("\n");
}

/* Recursive helper: print children range [start..end] (inclusive indices into node.children)
   as a left-associative binary tree where operators are the separators. This visually groups
   sequences like: cmd0 op cmd1 op cmd2  -> ((cmd0 op cmd1) op cmd2) with the last operator as root. */
static void print_sequence_range(t_ast_node *children, int start, int end,
								 int *depth_stack, int depth)
{
	int op_idx;

	/* Guard: invalid range */
	if (start > end || start < 0)
		return;

	/* single element: just print the child node normally */
	if (start == end)
	{
		if (depth_stack)
			depth_stack[depth] = 1;
		print_tree_recursive(children[start], depth_stack, depth);
		return;
	}

	/* For left-associative grouping, choose the last operator as the root of this subtree.
	   In alternating cmd op cmd op cmd sequences, operators are at odd indices.
	   However, if the list ends with an operator (e.g., "cmd &"), we need to handle that. */
	
	/* Find the last operator token in the range */
	op_idx = end;
	while (op_idx > start && children[op_idx].node_type != AST_TOKEN)
		op_idx--;
	
	/* If we couldn't find an operator, or range is malformed, just print sequentially */
	if (op_idx <= start || children[op_idx].node_type != AST_TOKEN)
	{
		int i = start;
		while (i <= end)
		{
			if (depth_stack)
				depth_stack[depth] = (i == end);
			print_tree_recursive(children[i], depth_stack, depth);
			i++;
		}
		return;
	}

	/* print operator node at this depth */
	print_tree_prefix(depth_stack, depth, depth > 0 && depth_stack[depth - 1]);
	print_op_line(children[op_idx]); /* children[op_idx] is the operator token */

	/* left subtree: range [start .. op_idx-1] */
	if (depth_stack)
		depth_stack[depth] = 0; /* operator has further siblings handling */
	print_sequence_range(children, start, op_idx - 1, depth_stack, depth + 1);

	/* right subtree: the operand after operator (if exists) */
	if (op_idx + 1 <= end)
	{
		if (depth_stack)
			depth_stack[depth] = 1; /* right child is last for this operator subtree */
		print_tree_recursive(children[op_idx + 1], depth_stack, depth + 1);
	}
}

/* Helper: Recursively print tree structure (collapses trivial pipeline wrapper and
   prints simple/compound lists as left-associative operator trees for clarity) */
static void print_tree_recursive(t_ast_node node, int *depth_stack, int depth)
{
	size_t i;
	int is_last;

	/* print branch/indent prefix for this depth */
	print_tree_prefix(depth_stack, depth,
					  depth > 0 && depth_stack[depth - 1]);

	/* Collapse trivial pipeline wrapper: show its single child in-place */
	if (node.node_type == AST_COMMAND_PIPELINE && node.children.len == 1)
	{
		t_ast_node child = *(t_ast_node *)vec_idx(&node.children, 0);
		/* print the child header at this same level */
		print_node_line(child);
		/* recurse into child's children (increase depth) */
		i = 0;
		while (i < child.children.len)
		{
			is_last = (i == child.children.len - 1);
			if (depth_stack)
				depth_stack[depth] = is_last;
			print_tree_recursive(*(t_ast_node *)vec_idx(&child.children, i),
								 depth_stack, depth + 1);
			i++;
		}
		return;
	}

	/* Special handling: present simple/compound list sequences as nested operator trees
	   rather than flat alternating lists to better reflect logical grouping. */
	if ((node.node_type == AST_SIMPLE_LIST || node.node_type == AST_COMPOUND_LIST) && node.children.len > 1)
	{
		/* Print this node header */
		print_node_line(node);
		/* prepare children array and call sequence printer on full range */
		t_ast_node *children = (t_ast_node *)node.children.ctx;
		if (depth_stack)
			depth_stack[depth] = 1; /* this node printed; its single subtree follows */
		print_sequence_range(children, 0, (int)node.children.len - 1, depth_stack, depth + 1);
		return;
	}

	/* Normal printing for non-collapsed nodes */
	print_node_line(node);
	i = 0;
	while (i < node.children.len)
	{
		is_last = (i == node.children.len - 1);
		if (depth_stack)
			depth_stack[depth] = is_last;
		print_tree_recursive(*(t_ast_node *)vec_idx(&node.children, i),
							 depth_stack, depth + 1);
		i++;
	}
}

/* Helper: Print formatted tree to console */
static void print_ast_tree(t_ast_node node)
{
	int depth_stack[256];
	int i;

	i = 0;
	while (i < 256)
		depth_stack[i++] = 0;
	ft_printf("\n\033[1;35m=== AST Tree Structure ===\033[0m\n\n");
	print_tree_recursive(node, depth_stack, 0);
	ft_printf("\n\033[1;35m=========================\033[0m\n\n");
}

/* Original function - enhanced with tree output */
char *node_name(t_ast_t tn)
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
	ft_assert("Unreachable" == 0);
	return (0);
}

/* Original function - kept as is */
void print_node(t_ast_node node)
{
	size_t i;

	ft_printf(" (%s", node_name(node.node_type));
	if (node.node_type == AST_TOKEN)
		ft_printf(" >%.*s<", node.token.len, node.token.start);
	i = 0;
	while (i < node.children.len)
		print_node(*(t_ast_node *)vec_idx(&node.children, i++));
	ft_printf(")");
}

/* Original function - kept as is */
void print_token_str(t_ast_node node, int outfd)
{
	int i;
	char c;

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

/* Enhanced DOT node printing with colors and shapes */
void print_dot_node(t_shell *state, t_ast_node node, uint32_t id, int outfd)
{
	size_t i;
	uint32_t r;

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
		print_dot_node(state, *(t_ast_node *)vec_idx(&node.children, i), r, outfd);
		i++;
	}
}

/* Enhanced DOT output with better styling */
void print_ast_dot(t_shell *state, t_ast_node node)
{
	int outfd;

	outfd = open("out.dot", O_WRONLY | O_TRUNC | O_CREAT, 0666);
	if (outfd < 0)
	{
		warning_error_errno();
		return;
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