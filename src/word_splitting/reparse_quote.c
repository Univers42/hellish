/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   reparser2.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 23:32:40 by marvin            #+#    #+#             */
/*   Updated: 2026/01/09 23:32:40 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../libft/libft.h"
#include "shell.h"
#include <stdbool.h>
# include "parser.h"

//[a-zA-Z_]
bool	is_var_name_p1(char c)
{
	if (ft_isalpha(c) || c == '_')
		return (true);
	return (false);
}

//[a-zA-Z0-9_]
bool	is_var_name_p2(char c)
{
	if (ft_isalnum(c) || c == '_')
		return (true);
	return (false);
}

bool	reparse_special_envvar(t_ast_node *ret, int *i, t_token t, t_tt tt)
{
	int		prev_start;
	char	c;

	prev_start = *i;
	c = t.start[*i];
	if (c != '?' && c != '$')
		return (false);
	(*i)++;
	{
		t_ast_node tmp = create_subtoken_node(t, prev_start, *i, tt);
		tmp.children.elem_size = sizeof(t_ast_node);
		vec_push(&ret->children, &tmp);
	}
	return (true);
}

void	reparse_envvar(t_ast_node *ret, int *i, t_token t, t_tt tt)
{
	int	prev_start;

	ft_assert(t.start[(*i)++] == '$');
	prev_start = *i;
	
	/* Handle $(( arithmetic expansion - treat the whole $((...)) as a single token */
	if (*i + 1 < t.len && t.start[*i] == '(' && t.start[*i + 1] == '(')
	{
		int depth = 2;
		(*i) += 2; /* skip (( */
		while (*i < t.len && depth > 0)
		{
			if (*i + 1 < t.len && t.start[*i] == '(' && t.start[*i + 1] == '(')
				{ depth += 2; (*i) += 2; }
			else if (*i + 1 < t.len && t.start[*i] == ')' && t.start[*i + 1] == ')')
				{ depth -= 2; (*i) += 2; }
			else if (t.start[*i] == '(')
				{ depth++; (*i)++; }
			else if (t.start[*i] == ')')
				{ depth--; (*i)++; }
			else
				(*i)++;
		}
		t_ast_node tmp = create_subtoken_node(t, prev_start - 1, *i, TT_WORD);
		tmp.children.elem_size = sizeof(t_ast_node);
		vec_push(&ret->children, &tmp);
		return;
	}
	
	/* Handle $() command substitution - treat the whole $() as a single token. */
	if (*i < t.len && t.start[*i] == '(')
	{
		int depth = 1;
		(*i)++; /* skip opening '(' */
		while (*i < t.len && depth > 0)
		{
			if (t.start[*i] == '(')
				depth++;
			else if (t.start[*i] == ')')
				depth--;
			(*i)++;
		}
		t_ast_node tmp = create_subtoken_node(t, prev_start - 1, *i, TT_WORD);
		tmp.children.elem_size = sizeof(t_ast_node);
		vec_push(&ret->children, &tmp);
		return;
	}
	if (reparse_special_envvar(ret, i, t, tt))
		return ;
	if (*i < t.len && is_var_name_p1(t.start[*i]))
		(*i)++;
	while (*i < t.len && is_var_name_p2(t.start[*i]))
	{
		(*i)++;
	}
	if (prev_start == *i)
	{
		if (tt == TT_DQENVVAR && prev_start < t.len && t.start[prev_start] == '"')
		{
			t_ast_node tmp = create_subtoken_node(t, prev_start - 1, prev_start, TT_DQWORD);
			tmp.children.elem_size = sizeof(t_ast_node);
			vec_push(&ret->children, &tmp);
		}
		else if (prev_start < t.len && (t.start[prev_start] == '\'' || t.start[prev_start] == '"'))
		{
			t_ast_node tmp = create_subtoken_node(t, prev_start, prev_start, TT_ENVVAR);
			tmp.children.elem_size = sizeof(t_ast_node);
			vec_push(&ret->children, &tmp);
		}
		else
		{
			t_tt literal_tt = (tt == TT_DQENVVAR) ? TT_DQWORD : TT_SQWORD;
			t_ast_node tmp = create_subtoken_node(t, prev_start - 1, *i, literal_tt);
			tmp.children.elem_size = sizeof(t_ast_node);
			vec_push(&ret->children, &tmp);
		}
	}
	else
	{
		t_ast_node tmp = create_subtoken_node(t, prev_start, *i, tt);
		tmp.children.elem_size = sizeof(t_ast_node);
		vec_push(&ret->children, &tmp);
	}
}

void reparse_dquote(t_ast_node *ret, int *i, t_token t)
{
	int prev_start;
	bool pushed_any;

	ft_assert(t.start[(*i)++] == '"');
	prev_start = *i;
	pushed_any = false;
	while (*i < t.len && t.start[*i] != '"')
	{
		if (t.start[*i] == '\\')
		{
			/* push chunk before backslash */
			if (*i > prev_start)
			{
				t_ast_node tmp = create_subtoken_node(t, prev_start, *i, TT_DQWORD);
				tmp.children.elem_size = sizeof(t_ast_node);
				vec_push(&ret->children, &tmp);
				pushed_any = true;
			}
			/* handle backslash in double quotes: remove backslash only when
			   followed by ", $, `, \, or newline; otherwise keep the backslash */
			int esc_pos = *i;
			(*i)++;
			if (*i < t.len)
			{
				char c = t.start[*i];
				if (c == '\n')
				{
					/* backslash-newline: line continuation -> consume both, emit nothing */
					(*i)++;
				}
				else if (c == '"' || c == '$' || c == '\\' || c == '`')
				{
					/* emit the escaped char without backslash as TT_DQWORD (literal).
					   This means \$ becomes a literal $ that won't be expanded. */
					t_ast_node tmp2 = create_subtoken_node(t, *i, *i + 1, TT_DQWORD);
					tmp2.children.elem_size = sizeof(t_ast_node);
					vec_push(&ret->children, &tmp2);
					pushed_any = true;
					(*i)++;
				}
				else
				{
					/* keep backslash as literal together with next char */
					t_ast_node tmp2 = create_subtoken_node(t, esc_pos, *i + 1, TT_DQWORD);
					tmp2.children.elem_size = sizeof(t_ast_node);
					vec_push(&ret->children, &tmp2);
					pushed_any = true;
					(*i)++;
				}
			}
			else
			{
				/* backslash at end: include it as literal */
				t_ast_node tmp2 = create_subtoken_node(t, esc_pos, *i, TT_DQWORD);
				tmp2.children.elem_size = sizeof(t_ast_node);
				vec_push(&ret->children, &tmp2);
				pushed_any = true;
			}
			prev_start = *i;
			continue;
		}
		else if (t.start[*i] == '$')
		{
			/* push chunk before $ */
			if (*i > prev_start)
			{
				t_ast_node tmp = create_subtoken_node(t, prev_start, *i, TT_DQWORD);
				tmp.children.elem_size = sizeof(t_ast_node);
				vec_push(&ret->children, &tmp);
				pushed_any = true;
			}
			reparse_envvar(ret, i, t, TT_DQENVVAR);
			pushed_any = true;
			prev_start = *i;
			continue;
		}
		(*i)++;
	}
	/* push remaining chunk inside quotes */
	if (*i > prev_start)
	{
		t_ast_node tmp = create_subtoken_node(t, prev_start, *i, TT_DQWORD);
		tmp.children.elem_size = sizeof(t_ast_node);
		vec_push(&ret->children, &tmp);
		pushed_any = true;
	}
	/* If nothing was pushed (empty quotes ""), push an empty DQWORD token
	   so the word node has at least one child representing the empty string */
	if (!pushed_any)
	{
		t_ast_node tmp = create_subtoken_node(t, *i, *i, TT_DQWORD);
		tmp.children.elem_size = sizeof(t_ast_node);
		vec_push(&ret->children, &tmp);
	}
	ft_assert(t.start[(*i)++] == '"');
}
