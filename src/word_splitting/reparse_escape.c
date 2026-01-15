/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   reparser3.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 23:32:46 by marvin            #+#    #+#             */
/*   Updated: 2026/01/09 23:32:46 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../libft/libft.h"
#include "shell.h"
# include "parser.h"
# include "decomposer.h"
# include "helpers.h"

void	reparse_words(t_ast_node	*node)
{
	t_ast_node	temp;
	t_token_old	full_word;
	t_token		tok;
	size_t		i;

	if (node->node_type == AST_WORD)
	{
		ft_assert(node->children.len == 1);
		tok = ((t_ast_node *)node->children.ctx)[0].token;
		full_word = (t_token_old){.start = tok.start,
			.len = tok.len, .present = true};
		temp = *node;
		*node = reparse_word(((t_ast_node *)temp.children.ctx)[0].token);
		// Save new children pointer and length
		void *new_ctx = node->children.ctx;
		size_t new_len = node->children.len;
		// Only free temp if its children buffer is different
		if (temp.children.ctx != new_ctx)
			free_ast(&temp);
		i = 0;
		while (i < new_len)
			((t_ast_node *)new_ctx)[i++].token.full_word = full_word;
	}
	i = 0;
	while (i < node->children.len)
		reparse_words(&((t_ast_node *)node->children.ctx)[i++]);
}

bool	is_valid_ident(char *s, int len)
{
	int			i;

	i = 0;
	if (len <= 0 || !is_var_name_p1(s[0]))
		return (false);
	while (i < len)
	{
		if (!is_var_name_p2(s[i++]))
			return (false);
	}
	return (true);
}

void	reparse_assignment_word(t_ast_node *word)
{
	t_ast_node	new_root;
	t_token		*first_token;
	char		*eq;

	/* Guard: ensure children exist and are valid before accessing */
	if (!word->children.ctx || word->children.len == 0)
		return;
	if (((t_ast_node *)word->children.ctx)[0].node_type != AST_TOKEN)
		return;

	new_root = (t_ast_node){.node_type = AST_ASSIGNMENT_WORD};
	vec_init(&new_root.children);
	new_root.children.elem_size = sizeof(t_ast_node);
	if (((t_ast_node *)word->children.ctx)[0].token.tt == TT_WORD)
	{
		first_token = &((t_ast_node *)word->children.ctx)[0].token;
		/* Guard: ensure token start is valid */
		if (!first_token->start || first_token->len <= 0)
			return;
		eq = ft_strnchr(first_token->start, '=', first_token->len);
		if (eq)
		{
			if (!is_valid_ident(first_token->start, eq - first_token->start))
				return ;
			{
				t_ast_node tmp = create_subtoken_node(*first_token, 0,
					eq - first_token->start, TT_WORD);
				vec_push(&new_root.children, &tmp);
			}
			first_token->len = first_token->len - (eq - first_token->start) - 1;
			first_token->start = eq + 1;
			vec_push(&new_root.children, word);
			*word = new_root;
		}
	}
}

void	reparse_assignment_words(t_ast_node *node)
{
	size_t	i;

	/* Guard: ensure node has valid children before recursing */
	if (!node->children.ctx)
		return;

	if (node->node_type != AST_REDIRECT)
	{
		i = 0;
		while (i < node->children.len)
			reparse_assignment_words(&((t_ast_node *)node->children.ctx)[i++]);
	}
	if (node->node_type == AST_WORD)
		reparse_assignment_word(node);
}

/* NOTE: reparse_dq_bs is defined in reparse_envar_plain.c - do not duplicate here */
