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

static t_token	*get_first_token_ptr(t_ast_node *word)
{
	if (!word->children.ctx || word->children.len == 0)
		return (NULL);
	if (((t_ast_node *)word->children.ctx)[0].node_type != AST_TOKEN)
		return (NULL);
	return (&((t_ast_node *)word->children.ctx)[0].token);
}

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
	vec_push(&new_root.children, word);
	*word = new_root;
}

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
	if (!is_valid_ident(first_token->start, eq_pos))
		return ;
	apply_assignment_split(word, first_token, eq_pos);
}

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
