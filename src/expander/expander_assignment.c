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
# include "env.h"
# include "expander.h"

t_string	word_to_string(t_ast_node node)
{
	t_token		curr;
	t_string	s;
	size_t		i;

	vec_init(&s);
	s.elem_size = 1;
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
			/* If start is present, push its contents (may be empty string for $''/$"").
			   If start is NULL and token is an envvar with len==0, emit literal '$'. */
			if (curr.start)
			{
				/* Push exactly curr.len bytes; zero-length means push nothing. */
				vec_push_nstr(&s, curr.start, (size_t)curr.len);
			}
			else if (curr.len == 0 && (curr.tt == TT_DQENVVAR || curr.tt == TT_ENVVAR))
			{
				/* If the word consists only of this envvar token (e.g. echo "$"),
				   treat it as a literal '$'. If there's additional children (e.g.
				   $'') then it's an explicit empty expansion and should be skipped. */
				if (node.children.len == 1)
				{
					char ch = '$';
					vec_push(&s, &ch);
				}
				/* else: explicit empty expansion (do nothing) */
			}
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
	args.elem_size = sizeof(char *);
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
		/* Duplicate the value so ownership is clear regardless of args.ctx */
		char *val = ((char **)args.ctx)[0];
		if (val)
		{
			ret.value = ft_strdup(val);
			free(val);
		}
		else
			ret.value = ft_strdup("");
		/* Trim leading '=' characters to mimic zsh behavior (aa==vv -> vv) */
		if (ret.value && ret.value[0] == '=')
		{
			size_t k = 0;
			while (ret.value[k] == '=')
				k++;
			char *trimmed = ft_strdup(ret.value + k);
			free(ret.value);
			ret.value = trimmed ? trimmed : ft_strdup("");
		}
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
