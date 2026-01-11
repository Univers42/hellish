/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   expander_assignment.c                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 23:31:10 by marvin            #+#    #+#             */
/*   Updated: 2026/01/09 23:31:10 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "libft.h"
#include "shell.h"
#include <stdio.h>

t_string	word_to_string(t_ast_node node)
{
	t_token		curr;
	t_string	s;
	size_t		i;

	vec_init(&s);
	i = 0;
	while (i < node.children.len)
	{
		if (!node.children.ctx)
			break;
		if (((t_ast_node *)node.children.ctx)[i].node_type != AST_TOKEN)
			break;
		curr = ((t_ast_node *)node.children.ctx)[i].token;
		if (curr.tt == TT_WORD || curr.tt == TT_SQWORD
			|| curr.tt == TT_DQWORD || curr.tt == TT_DQENVVAR
			|| curr.tt == TT_ENVVAR)
		{
			if (curr.start || !curr.len)
				vec_push_nstr(&s, curr.start, curr.len);
		}
		else
		{
			// skip unexpected tokens, do not crash
		}
		i++;
	}
	return (s);
}

t_string	word_to_hrdoc_string(t_ast_node node)
{
	t_token		curr;
	t_string	s;
	size_t		i;

	vec_init(&s);
	i = 0;
	while (i < node.children.len)
	{
		ft_assert(((t_ast_node *)node.children.ctx)[i].node_type == AST_TOKEN);
		curr = ((t_ast_node *)node.children.ctx)[i].token;
		if (curr.tt == TT_WORD || curr.tt == TT_SQWORD
			|| curr.tt == TT_DQWORD)
			vec_push_nstr(&s, curr.start, curr.len);
		else if (curr.tt == TT_DQENVVAR || curr.tt == TT_ENVVAR)
		{
			char ch = '$'; vec_push(&s, &ch);
			vec_push_nstr(&s, curr.start, curr.len);
		}
		else
			ft_assert("Unreachable" == 0);
		i++;
	}
	return (s);
}

t_env	assignment_to_env(t_shell *state, t_ast_node *node)
{
	t_vec	args;
	t_env		ret;

	ret = (t_env){.exported = false};
	vec_init(&args);
	ft_assert(node->children.len == 2);
	expand_word(state, &((t_ast_node *)node->children.ctx)[1], &args, true);
	ft_assert(((t_ast_node *)node->children.ctx)[1].children.ctx == 0);
	ret.key = ft_strndup(
		((t_ast_node *)node->children.ctx)[0].token.start,
		((t_ast_node *)node->children.ctx)[0].token.len
	);
	if (args.len)
	{
		ft_assert(args.len == 1);
		ret.value = ((char **)args.ctx)[0];
		if (!ret.value)
			ret.value = ft_strdup("");
	}
	free(args.ctx);
	return (ret);
}

void	assignment_word_to_word(t_ast_node *node)
{
	t_ast_node	ret;
	t_ast_node	left;
	t_ast_node	right;

	ret = (t_ast_node){.node_type = AST_WORD};
	vec_init(&ret.children);
	ret.children.elem_size = sizeof(t_ast_node);
	ft_assert(node->node_type == AST_ASSIGNMENT_WORD);
	ft_assert(node->children.len == 2);
	left = ((t_ast_node *)node->children.ctx)[0];
	right = ((t_ast_node *)node->children.ctx)[1];
	left.token.len++;
	vec_push(&ret.children, &left);
	//old vec_nd_push_vec, I think we can just use vec_push and we will be fine.
	vec_push(&ret.children, &right);
	/* right.children stays owned by pushed right; no double-free */
	/* free original node children buffer below */
	free(node->children.ctx);
	*node = ret;
}
