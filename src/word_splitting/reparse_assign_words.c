/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   reparse_assign_words.c                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 22:08:37 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/20 23:44:28 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "reparser_private.h"

/* Return a pointer to the first child token of a word node, or NULL if the
   node has no children or the first child is not a plain AST_TOKEN. This is
   where we verify the shape is what we expect before mutating it; bailing out
   on unexpected shapes avoids crashes on malformed ASTs during fuzzing. */
static t_token	*get_first_token_ptr(t_ast_node *word)
{
	if (!word->children.ctx || word->children.len == 0)
		return (NULL);
	if (((t_ast_node *)word->children.ctx)[0].node_type != AST_TOKEN)
		return (NULL);
	return (&((t_ast_node *)word->children.ctx)[0].token);
}

/* Scan for the first '=' in tok and return its offset. Returns -1 if absent
   or if the token is empty/NULL. We use ft_strnchr (bounded search) rather
   than strchr because tok->start is not necessarily NUL-terminated -- it is a
   slice into the raw input buffer. */
static int	find_eq_pos(t_token *tok)
{
	char	*eq;

	if (!tok || !tok->start || tok->len <= 0)
		return (-1);
	eq = ft_strnchr(tok->start, '=', tok->len);
	if (!eq)
		return (-1);
	return ((int)(eq - tok->start));
}

/* Mutate the AST_WORD node into an AST_ASSIGNMENT_WORD by inserting a new
   root node that carries the key name as a TT_WORD child, then the original
   word node (now representing the value) as a second child. We patch
   first_token->start and len in-place to trim off the "key=" prefix so the
   original raw text is not duplicated -- everything still points into the
   same input buffer. This is the cheapest correct way to split KEY=val
   without re-allocating the token strings. */
static void	apply_assignment_split(t_ast_node *word,
				t_token *first_token, int eq_pos)
{
	t_ast_node	new_root;

	new_root = (t_ast_node){.node_type = AST_ASSIGNMENT_WORD};
	vec_init(&new_root.children);
	new_root.children.elem_size = sizeof(t_ast_node);
	push_subtoken_node(&new_root, *first_token,
		create_interval(0, eq_pos), TT_WORD);
	first_token->len = first_token->len - (eq_pos)-1;
	first_token->start = first_token->start + eq_pos + 1;
	ast_push_child(&new_root, word);
	*word = new_root;
}

/* NAME[subscript] as an assignment left side: a valid identifier, an
   opening bracket, anything (an arithmetic expression) and the closing
   bracket exactly at the end — arr[i+1]=v assigns an array element. */
static bool	is_subscript_key(char *s, int len)
{
	int	i;

	i = 0;
	while (i < len && s[i] != '[')
		i++;
	if (i == 0 || i + 1 >= len || s[len - 1] != ']')
		return (false);
	return (is_valid_ident(s, i));
}

/* Attempt to re-classify one AST_WORD as an AST_ASSIGNMENT_WORD if it
   matches the pattern IDENT=value (or IDENT[expr]=value for an array
   element). Guard order matters: we check tt, then the '=' exists, then
   the left side is valid -- all must pass before we mutate the node.
   Early returns on any failure leave the node unchanged so normal word
   expansion picks it up. */
void	reparse_assignment_word(t_ast_node *word)
{
	t_token	*first_token;
	int		eq_pos;

	first_token = get_first_token_ptr(word);
	if (!first_token)
		return ;
	if (first_token->tt != TT_WORD)
		return ;
	if (!first_token->start || first_token->len <= 0)
		return ;
	eq_pos = find_eq_pos(first_token);
	if (eq_pos < 0)
		return ;
	if (!is_valid_ident(first_token->start, eq_pos)
		&& !is_subscript_key(first_token->start, eq_pos))
		return ;
	apply_assignment_split(word, first_token, eq_pos);
}

/* Recursively walk the AST and promote eligible word nodes to assignment
   words. We skip AST_REDIRECT subtrees (the filename in `>foo=bar` is not an
   assignment) and AST_PROC_SUB bodies (they are their own shell context).
   Every AST_WORD leaf gets tried last, after its children have been walked,
   matching the depth-first order that the POSIX spec implies. */
void	reparse_assignment_words(t_ast_node *node)
{
	size_t	i;

	if (!node->children.ctx)
		return ;
	if (node->node_type == AST_PROC_SUB)
		return ;
	if (node->node_type != AST_REDIRECT)
	{
		i = 0;
		while (i < node->children.len)
			reparse_assignment_words(&((t_ast_node *)node->children.ctx)[i++]);
	}
	if (node->node_type == AST_WORD)
		reparse_assignment_word(node);
}
