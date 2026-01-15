/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   reparser.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 23:32:35 by marvin            #+#    #+#             */
/*   Updated: 2026/01/09 23:32:35 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "shell.h"
#include <stdbool.h>
# include "libft.h"
#include <stddef.h>
# include "decomposer.h"
# include "lexer.h"
# include "ast.h"
# include "parser.h"

void	reparse_squote(t_ast_node *ret, int *i, t_token t)
{
	int	prev_start;

	ft_assert(t.start[(*i)++] == '\'');
	prev_start = *i;
	while (*i < t.len && t.start[*i] != '\'')
		(*i)++;
	/* Create SQWORD token for content inside single quotes - this is raw/literal */
	{
		t_ast_node tmp = create_subtoken_node(t, prev_start, *i, TT_SQWORD);
		tmp.children.elem_size = sizeof(t_ast_node);
		vec_push(&ret->children, &tmp);
	}
	ft_assert(t.start[(*i)++] == '\'');
}

void	reparse_bs(t_ast_node *ret, int *i, t_token t)
{
	int	prev_start;

	ft_assert(t.start[(*i)++] == '\\');
	prev_start = *i;
	if (*i == t.len)
	{
		/* backslash at end - include backslash as literal */
		prev_start--;
	}
	else
	{
		/* Outside quotes, backslash escapes the next character.
		   The escaped character becomes literal (use SQWORD to prevent further expansion). */
		(*i)++;
	}
	{
		t_ast_node tmp = create_subtoken_node(t, prev_start, *i, TT_SQWORD);
		tmp.children.elem_size = sizeof(t_ast_node);
		vec_push(&ret->children, &tmp);
	}
}

void	reparse_norm_word(t_ast_node *ret, int *i, t_token t)
{
	int	prev_start;

	prev_start = *i;
	while (*i < t.len && !is_special_char(t.start[*i]) && t.start[*i] != '\\')
		(*i)++;
	{
		t_ast_node tmp = create_subtoken_node(t, prev_start, *i, TT_WORD);
		tmp.children.elem_size = sizeof(t_ast_node);
		vec_push(&ret->children, &tmp);
	}
}

t_ast_node	reparse_word(t_token t)
{
	t_ast_node	ret;
	int			i;

	ret = (t_ast_node){.node_type = AST_WORD};
	vec_init(&ret.children);
	ret.children.elem_size = sizeof(t_ast_node);
	i = 0;
	while (i < t.len)
	{
		if (t.start[i] == '"')
			reparse_dquote(&ret, &i, t);
		else if (t.start[i] == '\\')
			reparse_bs(&ret, &i, t);
		else if (t.start[i] == '\'')
			reparse_squote(&ret, &i, t);
		else if (t.start[i] == '$')
			reparse_envvar(&ret, &i, t, TT_ENVVAR);
		else if (is_space(t.start[i]))
			ft_assert("Unreachable" == 0);
		else
			reparse_norm_word(&ret, &i, t);
	}
	return (ret);
}
