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

/* Abstract Syntax Tree types and node definition.
   Every parsed construct becomes a t_ast_node.  The tree is purely
   structural: it mirrors the grammar and carries raw tokens.  Expansion
   ($VAR, globs, heredocs) happens later in the executor/expander phases,
   not during parsing, so the same AST can be executed multiple times
   (e.g. inside a loop) without re-parsing. */

#ifndef AST_H
# define AST_H

# include "libft.h"
# include "public/token.h"

/* Forward declaration */
typedef struct s_shell	t_shell;

/* Node type: what grammar production this node represents.
   The executor switches on this to dispatch to the right handler. */
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
	AST_PROC_SUB,
	AST_IF,
	AST_WHILE,
	AST_UNTIL,
	AST_FOR,
	AST_CASE,
	AST_CASE_ITEM,
	AST_BRACE_GROUP,
	AST_FUNCTION_DEF,
	AST_ARITH_CMD,
	AST_FOR_ARITH,
	AST_ARRAY_ASSIGN,
	AST_COPROC
}	t_ast_type;

/* The universal AST node.  Used for EVERY grammar construct (commands,
   words, redirections, control structures).  children is a t_vec of
   t_ast_node (value, not pointer -- nodes are embedded). */
typedef struct s_ast_node
{
	t_ast_type		node_type; /* what this node is */
	t_token			token; /* the primary token (word text, op, etc.) */
	t_vec			children; /* child nodes (t_ast_node[], owned) */
	bool			has_redirect; /* true if any child is a redirection */
	int				redir_idx; /* index into state->redirects */
	bool			negate; /* true for `! cmd` (pipeline negation) */
	bool			glued; /* no whitespace separates this child from the one
						  before it, so it CONTINUES that word instead of
						  starting a new one.  Set on process substitutions
						  only, at parse time -- see procsub_assign.c */
	char			*heredoc_body; /* raw body string for a HERE-doc node */
}	t_ast_node;

/* Vector type alias for AST nodes */
typedef t_vec			t_vec_nd;

/* Function prototypes */
void		ast_push_child(t_ast_node *parent, t_ast_node *child);
void		free_ast(t_ast_node *node);
t_ast_node	clone_ast(t_ast_node *src);
t_ast_node	deep_clone_ast(t_ast_node *src);
void		ast_clone_scalars(t_ast_node *dst, t_ast_node *src);
void		ast_postorder_traversal(t_ast_node *node,
				void (*f)(t_ast_node *node));
void		print_ast_dot(t_shell *state, t_ast_node node);

/* Sanity ceiling on a recovered source span. Tokens from two different
   buffers would produce a nonsense distance; refuse rather than copy it. */
# define AST_SPAN_MAX 1048576

char		*ast_source_text(t_ast_node *node);
char		*node_name(t_ast_type tn);
void		print_node(t_ast_node node);

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