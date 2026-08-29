/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   reparse_assign_words2.c                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 22:08:37 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/20 23:44:28 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "reparser_private.h"

t_token	*get_first_token_ptr(t_ast_node *word);
int		find_eq_pos(t_token *tok);
void	apply_assignment_split(t_ast_node *word, t_token *first_token,
			int eq_pos);
bool	is_subscript_key(char *s, int len);
bool	is_append_key(char *s, int len);

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
		&& !is_subscript_key(first_token->start, eq_pos)
		&& !is_append_key(first_token->start, eq_pos)
		&& !is_zsh_pos_key(first_token->start, eq_pos))
		return ;
	apply_assignment_split(word, first_token, eq_pos);
}

/* Pre-pass (runs BEFORE reparse_words): a raw word matching NAME[...]=value
   is classified as an assignment while still a single token, so a '$' in
   the subscript (a[$i]=x, h[$key]=v) does not split the word out from
   under the classifier. Only words with a '[' before the '=' are touched;
   plain NAME=value is left to the normal post-split pass. subscript_assign
   later expands the raw subscript (arith for indexed, param for assoc). */
static void	subscript_assign_word(t_ast_node *word)
{
	t_token	*ft;
	int		eq;

	ft = get_first_token_ptr(word);
	if (!ft || ft->tt != TT_WORD || !ft->start || ft->len <= 0)
		return ;
	eq = find_eq_pos(ft);
	if (eq < 1 || !is_subscript_key(ft->start, eq))
		return ;
	if (!ft_strnchr(ft->start, '[', eq))
		return ;
	apply_assignment_split(word, ft, eq);
}

void	reparse_subscript_assigns(t_ast_node *node)
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
			reparse_subscript_assigns(
				&((t_ast_node *)node->children.ctx)[i++]);
	}
	if (node->node_type == AST_WORD)
		subscript_assign_word(node);
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
