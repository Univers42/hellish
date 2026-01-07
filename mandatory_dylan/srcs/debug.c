/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   debug.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/07 15:45:08 by marvin            #+#    #+#             */
/*   Updated: 2026/01/07 15:45:08 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../shell.h"
#include <fcntl.h>

/* ============== Token Type Name Lookup ============== */
#ifdef DEBUG_LEXER
char	*tt_to_str(t_tt tt)
{
	if (tt == TT_NONE)
		return ("TT_NONE");
	if (tt == TT_WORD)
		return ("TT_WORD");
	if (tt == TT_REDIRECT_LEFT)
		return ("TT_REDIRECT_LEFT");
	if (tt == TT_REDIRECT_RIGHT)
		return ("TT_REDIRECT_RIGHT");
	if (tt == TT_APPEND)
		return ("TT_APPEND");
	if (tt == TT_PIPE)
		return ("TT_PIPE");
	if (tt == TT_HEREDOC)
		return ("TT_HEREDOC");
	if (tt == TT_NEWLINE)
		return ("TT_NEWLINE");
	if (tt == TT_BRACE_LEFT)
		return ("TT_BRACE_LEFT");
	if (tt == TT_BRACE_RIGHT)
		return ("TT_BRACE_RIGHT");
	if (tt == TT_AND)
		return ("TT_AND");
	if (tt == TT_OR)
		return ("TT_OR");
	if (tt == TT_SEMICOLON)
		return ("TT_SEMICOLON");
	if (tt == TT_END)
		return ("TT_END");
	if (tt == TT_SQWORD)
		return ("TT_SQWORD");
	if (tt == TT_DQWORD)
		return ("TT_DQWORD");
	if (tt == TT_ENVVAR)
		return ("TT_ENVVAR");
	if (tt == TT_DQENVVAR)
		return ("TT_DQENVVAR");
	return ("TT_UNKNOWN");
}

void	print_tokens(t_deque *tokens)
{
	t_token	*curr;
	size_t	i;

	i = 0;
	ft_printf("------- PRINTING TOKENS --------\n");
	while (i < tokens->len)
	{
		curr = (t_token *)deque_idx(tokens, i);
		ft_printf("%s: >%.*s<\n", tt_to_str(curr->tt), (int)curr->len, curr->start);
		i++;
	}
	ft_printf("------- DONE --------\n");
}
#endif /* DEBUG_LEXER */

/* ============== AST Node Type Name Lookup ============== */
#ifdef DEBUG_AST
char	*node_name(t_ast_type tn)
{
	if (tn == AST_SIMPLE_LIST)
		return ("AST_SIMPLE_LIST");
	if (tn == AST_COMMAND_PIPELINE)
		return ("AST_COMMAND_PIPELINE");
	if (tn == AST_REDIRECT)
		return ("AST_REDIRECT");
	if (tn == AST_SIMPLE_COMMAND)
		return ("AST_SIMPLE_COMMAND");
	if (tn == AST_SUBSHELL)
		return ("AST_SUBSHELL");
	if (tn == AST_COMPOUND_LIST)
		return ("AST_COMPOUND_LIST");
	if (tn == AST_COMMAND)
		return ("AST_COMMAND");
	if (tn == AST_WORD)
		return ("AST_WORD");
	if (tn == AST_ASSIGNMENT_WORD)
		return ("AST_ASSIGNMENT_WORD");
	if (tn == AST_TOKEN)
		return ("AST_TOKEN");
	return ("AST_UNKNOWN");
}

/* ============== AST Node Printing ============== */

static void	print_node_recursive(t_ast_node *node)
{
	size_t	i;

	ft_printf(" (%s", node_name(node->node_type));
	if (node->node_type == AST_TOKEN)
	{
		ft_printf(" [token]");
	}
	i = 0;
	while (i < node->children.len)
	{
		print_node_recursive((t_ast_node *)vec_ctx_idx(&node->children, i));
		i++;
	}
	ft_printf(")");
}

void	print_node(t_ast_node *node)
{
	if (!node)
		return ;
	print_node_recursive(node);
	ft_printf("\n");
}

/* ============== Token String Escaping (for DOT output) ============== */

static void	print_token_str_escaped(const char *start, size_t len, int outfd)
{
	size_t	i;
	char	c;

	i = 0;
	while (i < len)
	{
		c = start[i];
		if (c == '\\')
			ft_fdprintf(outfd, "\\\\");
		else if (c == '"')
			ft_fdprintf(outfd, "\\\"");
		else if (c == '\'')
			ft_fdprintf(outfd, "\\'");
		else if (c == '\n')
			ft_fdprintf(outfd, "\\n");
		else if (c == '\t')
			ft_fdprintf(outfd, "\\t");
		else
			ft_fdprintf(outfd, "%c", c);
		i++;
	}
}

/* ============== DOT Graph Output ============== */

static void	print_dot_node_recursive(t_prng_state *prng, t_ast_node *node, uint32_t node_id, int outfd)
{
	size_t	i;
	uint32_t child_id;

	ft_fdprintf(outfd, "\tn%u [label=\"%s", node_id, node_name(node->node_type));
	if (node->node_type == AST_TOKEN)
	{
		ft_fdprintf(outfd, " [token]");
	}
	ft_fdprintf(outfd, "\"];\n");
	i = 0;
	while (i < node->children.len)
	{
		child_id = random_uint32(prng);
		ft_fdprintf(outfd, "\tn%u -> n%u;\n", node_id, child_id);
		print_dot_node_recursive(prng, (t_ast_node *)vec_ctx_idx(&node->children, i), child_id, outfd);
		i++;
	}
}

void	print_ast_dot(t_prng_state *prng, t_ast_node *node)
{
	int	outfd;

	if (!node || !prng)
		return ;
	outfd = open("ast_out.dot", O_WRONLY | O_TRUNC | O_CREAT, 0666);
	if (outfd < 0)
	{
		warning_error_errno();
		return ;
	}
	ft_fdprintf(outfd, "digraph AST {\n");
	print_dot_node_recursive(prng, node, 0, outfd);
	ft_fdprintf(outfd, "}\n");
	close(outfd);
	ft_printf("AST graph written to ast_out.dot\n");
}
#endif /* DEBUG_AST */

